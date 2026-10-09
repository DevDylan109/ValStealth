# ValStealth (LolChat Blocker)

Tiny native Windows tray app (~1-2 MB RAM). While running, it blocks outbound TCP port 5223 with a firewall rule named `lolchat`. Exiting removes the rule. No window, no console, only a tray icon.

## Files (keep in one folder)

- `lolchat.c` - source
- `lolchat.rc` - embeds the icon
- `icon.ico` - your icon (must have this exact name; use a multi-size .ico: 16, 32, 48, 256)

## Build

1. Install **Build Tools for Visual Studio** with the **Desktop development with C++** workload.
2. Open **x64 Native Tools Command Prompt for VS** (Start menu).
3. Go to the folder and run:

```
cd C:\path\to\folder
rc lolchat.rc
cl /O1 /GS- /W3 lolchat.c lolchat.res /Fe:ValStealth.exe /link /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup user32.lib shell32.lib advapi32.lib
```

No icon yet? Skip `rc` and the `.res` file:

```
cl /O1 /GS- /W3 lolchat.c /Fe:ValStealth.exe /link /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup user32.lib shell32.lib advapi32.lib
```

Output: `ValStealth.exe` (you can delete the `.obj` and `.res` files).

## Run

- Double-click `ValStealth.exe` and accept the UAC prompt (admin is required).
- A tray icon appears (it may be under the `^` arrow). The port is now blocked.
- Quit: right-click the tray icon > **Exit**. The firewall rule is deleted.
- The app does not start with Windows. Launch it manually each time.

## Verify

In an admin Command Prompt:

```
netsh advfirewall firewall show rule name="lolchat"
```

The rule shows while the app runs and is gone after you exit.

## Troubleshooting

- **`cl` / `rc` not recognized:** you are not in the x64 Native Tools prompt.
- **Cannot open `lolchat.res`:** run `rc lolchat.rc` first, from the same folder as `icon.ico`.
- **Unresolved `OpenProcessToken` / `GetTokenInformation`:** add `advapi32.lib` to the link line (included above).
- **Rule stuck after a crash or Task Manager kill:** it is cleared on the next start, or run:
  `netsh advfirewall firewall delete rule name="lolchat"`
- **Old autostart task still present:** `schtasks /delete /tn LolChatBlocker /f` (and `/tn ValStealth` if you used that name).
