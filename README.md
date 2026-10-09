# ValStealth

Tiny Windows tray app that hides your account status while playing valorant or other riot games. While running, it blocks outbound TCP port 5223 (used for LoL chat) with a firewall rule named `lolchat`. Exiting removes the rule.

## Usage
- You have to run it as admin, once it's started you can find it in your system tray.
- It can take about 15-30 seconds for the result to be visible.
- It is recommended to start the app before you login.
- You will appear as offline to your friends and the chat function will be disabled.
- Exit the app via the tray, don't close it via the task manager!

## Troubleshooting

- **Rule stuck after a crash or Task Manager kill:** it is cleared on the next start, or run:
  `netsh advfirewall firewall delete rule name="lolchat"`
