#!/usr/bin/env python3
#
# fake_ecu.py - Simulated SSM2 engine control unit for testing FreeSSM without a car
#
# Copyright (C) 2026 Jacob Seman
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

"""
Opens a pseudo-terminal and answers SSM2 requests (ISO-14230 framing, as used
with serial pass-through / KKL interfaces) with synthetic, moving engine data.

FreeSSM only lists serial ports named /dev/ttyS*, /dev/ttyUSB*, /dev/ttyACM* or
/dev/rfcomm*, so the pseudo-terminal needs a symlink with such a name:

    ./tools/fake_ecu.py                  # creates /dev/ttyUSB9 (asks for sudo)
    ./tools/fake_ecu.py --no-link        # only prints the pseudo-terminal path

Then select "Serial Pass-Through" and the link name in the FreeSSM preferences
and connect to the engine control unit.
"""

import argparse
import fcntl
import math
import os
import select
import signal
import subprocess
import sys
import termios
import time
import tty

TESTER_ADDR = 0xF0
ECU_ADDR = 0x10
BAUDRATE = 4800

SYS_ID = bytes([0xA2, 0x10, 0x01])		# Ax 10 xx => flagbyte definitions; xx = 01 => "2.5L SOHC"
ROM_ID = bytes([0x1D, 0x12, 0x00, 0x07, 0x05])
NUM_FLAGBYTES = 48

# Supported measuring blocks as (flagbyte, flagbit), numbered from 1 like in
# SSMFlagbyteDefinitions_*.cpp:
MB_FLAGS = [
	(1, 1),	# Engine Speed
	(1, 2),	# Manifold Absolute Pressure
	(1, 5),	# Air/Fuel Learning #1
	(1, 6),	# Air/Fuel Correction #1
	(1, 7),	# Coolant Temperature
	(1, 8),	# Engine Load
	(2, 2),	# Rear O2 Sensor Voltage
	(2, 4),	# Throttle Opening Angle
	(2, 6),	# Intake Air Temperature
	(2, 7),	# Ignition Timing
	(2, 8),	# Vehicle Speed
	(3, 1),	# Atmosphere Pressure
	(3, 2),	# Knocking Correction
	(3, 4),	# Fuel Injection #1 Pulse
	(3, 6),	# Throttle Sensor Voltage
	(3, 8),	# Battery Voltage
	(4, 1),	# Front O2 Sensor #1 Heater Current
	(4, 2),	# Fuel Temperature
	(4, 4),	# Learned Ignition Timing
	(4, 6),	# Fuel Tank Pressure
	(4, 8),	# Manifold Relative Pressure
	(5, 2),	# Canister Purge Control (CPC) Valve Duty Ratio
	(5, 6),	# Fuel Level Sensor Voltage
	(5, 8),	# Rear O2 Sensor Heater Current
	(6, 4),	# Idle Speed Control (ISC) Valve Steps
	(8, 4),	# Air/Fuel Correction #3
	(8, 6),	# Air/Fuel Sensor #1 Lambda
]

# Supported switches as (flagbyte, flagbit); the data bit at the switch address
# has the same bit number:
SW_FLAGS = [
	(12, 2), (12, 3), (12, 5), (12, 6), (12, 7),	# addr 0x61: test mode connector, D-check, read memory, test mode, AT vehicle
	(13, 2), (13, 3), (13, 4), (13, 7), (13, 8),	# addr 0x62: A/C, power steering, ignition, idle, neutral
	(14, 1), (14, 3), (14, 5), (14, 6), (14, 7),	# addr 0x63: electrical load, knock #1, rear O2 rich, front O2 #1 rich, starter
	(15, 1), (15, 5), (15, 6), (15, 7), (15, 8),	# addr 0x64: A/C mid pressure, blower fan, rear defogger, camshaft, crankshaft
	(16, 2), (16, 4), (16, 5), (16, 6), (16, 8),	# addr 0x65: CPC solenoid, fuel pump relay, radiator fan relays #2 + #1, A/C compressor
]


def build_flagbytes():
	flagbytes = bytearray(NUM_FLAGBYTES)
	for byte, bit in MB_FLAGS + SW_FLAGS:
		flagbytes[byte - 1] |= 1 << (bit - 1)
	return bytes(flagbytes)


def checksum(data):
	return sum(data) & 0xFF


def clamp_u8(value):
	return max(0, min(255, int(round(value))))


def smoothstep(x):
	x = max(0.0, min(1.0, x))
	return x * x * (3 - 2 * x)


class EngineModel:
	"""Synthetic engine data: a warm-up followed by a repeating idle / rev / overrun cycle."""

	CYCLE = 24.0	# [s]

	def __init__(self):
		self._t0 = time.monotonic()
		self._written = {}

	def write(self, addr, value):
		self._written[addr] = value & 0xFF

	def snapshot(self):
		"""Returns the memory content (addr => byte) for the current point in time."""
		t = time.monotonic() - self._t0
		p = t % self.CYCLE
		# Rev (0...1): idle until 10 s, up until 12 s, hold until 15 s, down until 18 s
		if p < 10:
			rev = 0.0
		elif p < 12:
			rev = smoothstep((p - 10) / 2)
		elif p < 15:
			rev = 1.0
		elif p < 18:
			rev = 1 - smoothstep((p - 15) / 3)
		else:
			rev = 0.0
		throttle = smoothstep((p - 9.7) / 1.5) if p < 15 else 1 - smoothstep((p - 15) / 0.4)
		overrun = math.sin(math.pi * (p - 15) / 3) if 15 <= p < 18 else 0.0
		wobble = math.sin(2 * math.pi * t / 2.5)	# closed loop mixture oscillation

		rpm = 850 + 2650 * rev + 12 * math.sin(2 * math.pi * 1.3 * t)
		atm = 85
		manifold = 29 + 37 * throttle - 11 * overrun
		coolant = 20 + 68 * (1 - math.exp(-t / 90)) + 1.5 * math.sin(2 * math.pi * t / 40) * (1 - math.exp(-t / 200))
		intake_air = 28 + 12 * (1 - math.exp(-t / 200)) - 4 * rev
		correction = 4.0 * wobble
		learning = 1.6 + 0.8 * math.sin(2 * math.pi * t / 120)
		lambda1 = 1.0 - 0.03 * wobble - 0.08 * throttle * (1 - rev) + 0.25 * overrun
		rear_o2 = max(0.05, min(0.90, 0.45 + 0.40 * math.sin(2 * math.pi * t / 9) - 0.35 * overrun))
		heater_front = 1.6 + 4.4 * math.exp(-t / 45) + 0.1 * wobble
		heater_rear = 0.94 + 0.02 * math.sin(2 * math.pi * t / 7)
		battery = 13.8 + 0.1 * math.sin(2 * math.pi * t / 5) + 0.2 * rev
		ignition = 10 + 24 * rev - 6 * throttle * (1 - rev)
		knock = -1.5 if (12.5 < p < 13.5) else 0.0
		injection = 2.3 + 3.6 * throttle - 2.0 * overrun
		isc_steps = 52 + 63 * rev
		load = 18 + 45 * throttle - 8 * overrun
		purge = 0 if coolant < 60 else 18 + 20 * rev
		tank_pressure = 0.3 * math.sin(2 * math.pi * t / 30)
		fuel_temp = 22 + 8 * (1 - math.exp(-t / 600))

		rpm_raw = max(0, min(0xFFFF, int(round(rpm * 4))))
		rear_o2_raw = max(0, min(0xFFFF, int(round(rear_o2 * 200))))
		mem = {
			0x07: clamp_u8(load * 255 / 100),
			0x08: clamp_u8(coolant + 40),
			0x09: clamp_u8(correction * 128 / 100 + 128),
			0x0A: clamp_u8(learning * 128 / 100 + 128),
			0x0D: clamp_u8(manifold),
			0x0E: rpm_raw >> 8,
			0x0F: rpm_raw & 0xFF,
			0x10: 0,
			0x11: clamp_u8(ignition * 2 + 128),
			0x12: clamp_u8(intake_air + 40),
			0x15: clamp_u8((2 + 38 * throttle) * 255 / 100),
			0x18: rear_o2_raw >> 8,
			0x19: rear_o2_raw & 0xFF,
			0x1C: clamp_u8(battery * 25 / 2),
			0x1E: clamp_u8((0.5 + 3.4 * throttle) * 50),
			0x20: clamp_u8(injection * 1000 / 256),
			0x22: clamp_u8(knock * 2 + 128),
			0x23: clamp_u8(atm),
			0x24: clamp_u8(manifold - atm + 128),
			0x26: clamp_u8(tank_pressure * 40 + 128),
			0x28: clamp_u8(2.0 * 2 + 128),
			0x2A: clamp_u8(fuel_temp + 40),
			0x2B: clamp_u8(heater_front * 255 / 10),
			0x2C: clamp_u8(heater_rear * 255 / 10),
			0x2E: clamp_u8(2.6 * 50),
			0x32: clamp_u8(purge * 255 / 100),
			0x38: clamp_u8(isc_steps),
			0x46: clamp_u8(lambda1 * 128),
			0xD0: 128,
			# Switches:
			0x61: 0x00,
			0x62: 0x08 | 0x80 | (0x40 if throttle < 0.02 else 0),	# ignition, neutral, idle
			0x63: (0x10 if rear_o2 > 0.45 else 0) | (0x20 if lambda1 < 1.0 else 0) | (0x04 if knock else 0),
			0x64: 0x40 | 0x80,	# camshaft + crankshaft signal
			0x65: 0x08 | (0x02 if purge else 0) | (0x20 if coolant > 86 else 0),	# fuel pump relay, CPC, fan #1
		}
		mem.update(self._written)
		return mem


class FakeECU:
	def __init__(self, master_fd, model, echo, realtime, verbose):
		self._fd = master_fd
		self._model = model
		self._echo = echo
		self._realtime = realtime
		self._verbose = verbose
		self._flagbytes = build_flagbytes()
		self._buffer = bytearray()
		self._last_rx = 0.0
		self.requests = 0

	def feed(self, data):
		now = time.monotonic()
		if now - self._last_rx > 0.5:
			self._buffer.clear()	# drop stale partial message
		self._last_rx = now
		self._buffer += data
		while self._process_buffer():
			pass

	def _process_buffer(self):
		buf = self._buffer
		# Search header:
		while buf and buf[0] != 0x80:
			del buf[0]
		if len(buf) < 4:
			return False
		msg_len = 4 + buf[3] + 1
		if buf[3] == 0:
			del buf[0]
			return True
		if len(buf) < msg_len:
			return False
		msg = bytes(buf[:msg_len])
		if checksum(msg[:-1]) != msg[-1]:
			del buf[0]	# not a message start, resync
			return True
		del buf[:msg_len]
		if self._verbose:
			print("rx: " + msg.hex(" "))
		if (msg[1] != ECU_ADDR) or (msg[2] != TESTER_ADDR):
			return True	# message for another control unit => no answer
		if self._echo:
			os.write(self._fd, msg)	# K-line: the tester receives its own message
		response = self._handle_request(msg[4:-1])
		if response is not None:
			self._send(response)
		return True

	def _handle_request(self, req):
		cmd = req[0]
		mem = self._model.snapshot()
		if cmd == 0xBF and len(req) == 1:	# get control unit data
			return b"\xFF" + SYS_ID + ROM_ID + self._flagbytes
		if cmd == 0xA8 and len(req) >= 5 and (len(req) - 2) % 3 == 0:	# read multiple data bytes
			addrs = [int.from_bytes(req[k:k + 3], "big") for k in range(2, len(req), 3)]
			return b"\xE8" + bytes(mem.get(a, 0) for a in addrs)
		if cmd == 0xA0 and len(req) == 6:	# read data block
			addr = int.from_bytes(req[2:5], "big")
			return b"\xE0" + bytes(mem.get(addr + k, 0) for k in range(req[5] + 1))
		if cmd == 0xB8 and len(req) == 5:	# write data byte
			self._model.write(int.from_bytes(req[1:4], "big"), req[4])
			return b"\xF8" + req[4:5]
		if cmd == 0xB0 and len(req) >= 5:	# write data block
			addr = int.from_bytes(req[1:4], "big")
			for k, value in enumerate(req[4:]):
				self._model.write(addr + k, value)
			return b"\xF0" + req[4:]
		if self._verbose:
			print("unsupported request: " + req.hex(" "))
		return None

	def _send(self, data):
		msg = bytes([0x80, TESTER_ADDR, ECU_ADDR, len(data)]) + data
		msg += bytes([checksum(msg)])
		if self._realtime:
			time.sleep(0.005 + len(msg) * 10 / BAUDRATE)	# transmission time on a real K-line
		os.write(self._fd, msg)
		self.requests += 1
		if self._verbose:
			print("tx: " + msg.hex(" "))


def create_link(link, target):
	if os.path.lexists(link) and not os.path.islink(link):
		sys.exit("error: %s exists and is not a symlink, refusing to replace it" % link)
	try:
		if os.path.islink(link):
			os.unlink(link)
		os.symlink(target, link)
		return True
	except OSError:
		pass
	print("Creating %s -> %s (needs root):" % (link, target))
	return subprocess.call(["sudo", "ln", "-sfn", target, link]) == 0


def remove_link(link, target):
	if not (os.path.islink(link) and os.readlink(link) == target):
		return
	try:
		os.unlink(link)
	except OSError:
		if subprocess.call(["sudo", "-n", "rm", "-f", link], stderr=subprocess.DEVNULL) != 0:
			print("note: could not remove %s (remove it with: sudo rm %s)" % (link, link))


def main():
	parser = argparse.ArgumentParser(description="Simulated SSM2 engine control unit for testing FreeSSM without a car")
	parser.add_argument("--link", default="/dev/ttyUSB9", help="symlink to the pseudo-terminal, selectable in FreeSSM (default: %(default)s)")
	parser.add_argument("--no-link", action="store_true", help="do not create a symlink")
	parser.add_argument("--no-echo", action="store_true", help="do not echo requests (a real K-line interface does)")
	parser.add_argument("--fast", action="store_true", help="answer immediately instead of simulating the 4800 baud transmission time")
	parser.add_argument("-v", "--verbose", action="store_true", help="print all messages")
	args = parser.parse_args()

	signal.signal(signal.SIGTERM, lambda signum, frame: sys.exit(0))
	master_fd, slave_fd = os.openpty()
	tty.setraw(slave_fd)
	slave_name = os.ttyname(slave_fd)

	port = slave_name
	linked = False
	if not args.no_link:
		linked = create_link(args.link, slave_name)
		if linked:
			port = args.link
		else:
			print("warning: no symlink created, FreeSSM will not list %s" % slave_name)
	print("Fake ECU (ROM-ID %s) is listening on %s" % (ROM_ID.hex(" ").upper(), port))
	print("Press Ctrl+C to stop.")

	ecu = FakeECU(master_fd, EngineModel(), not args.no_echo, not args.fast, args.verbose)
	last_report = time.monotonic()
	last_count = 0
	try:
		while True:
			readable, _, _ = select.select([master_fd], [], [], 0.05)
			fcntl.ioctl(slave_fd, termios.TIOCNXCL)	# FreeSSM sets TIOCEXCL, which outlives its close on a pseudo-terminal
			if readable:
				data = os.read(master_fd, 4096)
				if data:
					ecu.feed(data)
			now = time.monotonic()
			if not args.verbose and (now - last_report >= 5):
				if ecu.requests != last_count:
					print("%d requests answered (%.1f/s)" % (ecu.requests, (ecu.requests - last_count) / (now - last_report)))
				last_report = now
				last_count = ecu.requests
	except KeyboardInterrupt:
		print()
	finally:
		if linked:
			remove_link(args.link, slave_name)
		os.close(slave_fd)
		os.close(master_fd)


if __name__ == "__main__":
	main()
