"""Developer-only: inspect the shipped PE, including its embedded manifest."""
import struct
import sys
from pathlib import Path
import xml.etree.ElementTree as ET


def check(path):
    b = Path(path).read_bytes()
    u16 = lambda o: struct.unpack_from('<H', b, o)[0]
    u32 = lambda o: struct.unpack_from('<I', b, o)[0]
    pe = u32(0x3c)
    assert b[:2] == b'MZ' and b[pe:pe + 4] == b'PE\0\0'
    assert u16(pe + 4) == 0x14c, 'expected native x86'
    opt = pe + 24
    assert u16(opt) == 0x10b
    assert (u32(opt + 96 + 14 * 8), u32(opt + 100 + 14 * 8)) == (0, 0), 'CLR dependency'
    sections = pe + 24 + u16(pe + 20)

    def offset(rva):
        for i in range(u16(pe + 6)):
            s = sections + i * 40
            start, size = u32(s + 12), max(u32(s + 8), u32(s + 16))
            if start <= rva < start + size:
                return u32(s + 20) + rva - start
        raise AssertionError('unmapped RVA')

    base = offset(u32(opt + 96 + 2 * 8))

    def entries(relative):
        d = base + relative
        return [(u32(d + 16 + i * 8), u32(d + 20 + i * 8))
                for i in range(u16(d + 12) + u16(d + 14))]

    def child(relative, key):
        matches = [target for name, target in entries(relative) if name == key]
        assert len(matches) == 1 and matches[0] & 0x80000000
        return matches[0] & 0x7fffffff

    manifests = entries(child(child(0, 24), 1))
    assert manifests, 'missing executable manifest'
    ns = {'a': 'urn:schemas-microsoft-com:asm.v1', 't': 'urn:schemas-microsoft-com:asm.v3'}
    for language, target in manifests:
        assert not target & 0x80000000
        d = base + target
        data = offset(u32(d))
        root = ET.fromstring(b[data:data + u32(d + 4)])
        assert root.find('a:dependency', ns) is None, 'unexpected Side-by-Side dependency'
        identity = root.find('a:assemblyIdentity', ns)
        assert identity is not None and identity.get('version') == '6.7.0.0', 'wrong app version'
        assert identity.get('processorArchitecture') == 'x86'
        level = root.find('.//t:requestedExecutionLevel', ns)
        assert level is not None and level.get('level') == 'asInvoker' and level.get('uiAccess') == 'false'

    allowed = {'advapi32.dll', 'comctl32.dll', 'comdlg32.dll', 'gdi32.dll',
               'kernel32.dll', 'msvcrt.dll', 'shell32.dll', 'user32.dll'}
    imports = set()
    d = offset(u32(opt + 96 + 8))
    while any(b[d:d + 20]):
        name = offset(u32(d + 12))
        imports.add(b[name:b.index(b'\0', name)].decode('ascii').lower())
        d += 20
    assert imports == allowed, 'unexpected DLL imports: ' + repr(imports)
    print('PASS: native x86, no CLR, OS DLLs only; embedded manifest version/privilege/XML and no SxS dependencies')


if __name__ == '__main__':
    check(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / 'WorkLookup.exe')
