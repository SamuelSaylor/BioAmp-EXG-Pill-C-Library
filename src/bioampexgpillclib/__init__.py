"""Fast BioAmp EXG Pill (ADS1115) reader. Linux only; the real work happens in C."""
from . import _core

__all__ = ["BioAmp", "SAMPLE_RATE"]

SAMPLE_RATE = 128  # Hz, matches the DR bits written by the C library's init


class BioAmp:
    def __init__(self, device="/dev/i2c-1", address=0x48):
        self._fd = _core.open(device, address)

    def read_raw(self):
        """One raw signed 16 bit count."""
        return _core.read_raw(self._fd)

    def read_voltage(self):
        """One sample in volts."""
        return _core.counts_to_voltage(_core.read_raw(self._fd))

    def read_block(self, n, volts=False, rate=SAMPLE_RATE):
        """Read n samples in one call (GIL released while reading).

        Returns array('h') of counts, or array('d') of volts if volts=True.
        `rate` paces the reads in Hz; use 0 to read as fast as possible
        (the ADS1115 will return repeated values faster than SAMPLE_RATE).
        """
        return _core.read_block(self._fd, n, float(rate), bool(volts))

    def close(self):
        if self._fd is not None:
            fd, self._fd = self._fd, None
            _core.close(fd)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def __del__(self):
        try:
            self.close()
        except Exception:
            pass
