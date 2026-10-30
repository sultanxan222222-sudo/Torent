# PS4 Pro firmware 13.02 — browser chain and HEN compatibility notes

Research snapshot: 2026-10-30. This document records public upstream/community documentation. It does **not** identify the exact payload currently loaded on the user's console.

## Keep the layers separate

A browser jailbreak chain, a kernel exploit/patch, and the homebrew payload (HEN/GoldHEN) are distinct components. A page saying that firmware 13.02 is "supported" does not, by itself, prove which HEN binary/version is installed or which optional services it exposes.

## What public documentation currently says

1. **Browser/kernel chain for 13.02–13.52:** the PS4 Developer Wiki records a SlopKit WebKit chain ported to PS4 by raw13g, plus the AIO kernel exploit chain called Relapse for 13.02–13.52. It credits raw13g for the PS4 chain/offset work and distinguishes this from the PS5 research.  
   Reference: https://www.psdevwiki.com/ps4/Vulnerabilities

2. **Independent implementation notes:** the public caziques/PS4 repository describes a browser-delivered chain for 13.02–13.52, credits raw13g for the underlying exploit chain, and lists 13.02 with a PS4 HEN payload marked upstream. It separately lists GoldHEN 2.4b18.11 for 13.52. This is a third-party test repository, not an official GoldHEN release.  
   Reference: https://github.com/caziques/PS4

3. **GoldHEN upstream:** the official GoldHEN project credits SiSTR0 as its author and documents features including BinLoader on port 9090, FTP on 2121, plugins, and internal package installation support at `/data/pkg`. The official release page's listed supported firmware set for v2.4b18 ends at 11.00; it does not establish official upstream v2.4b18 support for 13.02.  
   References: https://github.com/GoldHEN/GoldHEN and https://github.com/GoldHEN/GoldHEN/releases

4. **Third-party host claim:** HenJB's public page currently labels its interface GoldHEN v2.4b18.11 and advertises browser jailbreak support for 13.02, 13.04, 13.50, and 13.52. That is evidence that a third-party host offers a 13.02 flow; it is not proof that the user's console has that exact payload/version loaded.  
   Reference: https://henjb.com/

5. **Community compatibility overview:** ConsoleMods describes the browser-based 13.02–13.52 path as Relapse and notes that the jailbreak must be rerun after a reboot.  
   Reference: https://consolemods.org/wiki/PS4:FAQ

## Engineering conclusion for VAYNAX TORRENT

- Target the user's known device/firmware: PS4 Pro, firmware 13.02, browser-launched jailbreak.
- Do not label the user's installed payload as GoldHEN 2.4b18.11 or 2.4b18.12 based only on firmware or a host's banner.
- Treat the running HEN payload as an unknown runtime until a diagnostic run confirms its services.
- The current vendored client has hard-coded assumptions: BinLoader at 127.0.0.1:9090 and GoldHEN AutoRun configuration at `/data/GoldHEN/payloads.ini`. These assumptions must be isolated behind capability checks before they are used on this target.
- Do not write to `payloads.ini`, copy an autorun payload, or rely on the BinLoader service until the actual environment confirms the expected path, protocol, and permission.
- Probe non-destructively first: firmware/runtime identity where available, loopback TCP reachability for documented service ports, writable application/data directories, internal storage mount/free-space, and available package-install mechanisms. A closed port means "not detected", not permission to install or modify another payload.
- Use a capability matrix so torrent downloads, UI, and local media can run independently of optional GoldHEN-specific features.
- Console verification remains required. A host-side source audit cannot prove the user's actual payload version.

## First diagnostic test plan

1. Launch a minimal diagnostic build without changing HEN configuration.
2. Record only capability results (available/unavailable/error); never log user tokens or private data.
3. Verify writable internal storage and create a harmless temporary test file, then remove only that test file.
4. Test BinLoader availability without sending a payload.
5. Detect package-install support without attempting an install.
6. Save a diagnostic report for the user to read; only then enable compatible integration paths.

## References and attribution

- SiSTR0 — GoldHEN project.
- raw13g — PS4 13.02–13.52 browser/kernel exploit-chain implementation and firmware offsets, as credited by public PS4 developer documentation.
- TheFlow and other researchers — underlying kernel vulnerability research as credited by the developer documentation.
- ConsoleMods and PS4 Developer Wiki — community-maintained compatibility and vulnerability documentation.

This document is a compatibility audit, not an endorsement of third-party exploit hosts. Use only on hardware the owner controls; kernel exploits can crash the console.
