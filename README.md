# Aegis Password Vault v1.0.1

A CLI password manager with encrypted storage, search, and clipboard copy.

## Features
- Encrypted storage in %APPDATA%\Aegis\data\passwords.txt
- Add, edit, list, show, delete entries
- Search by website or account
- Password generator
- Clipboard copy
- Lock/unlock with master password

## Installation (Windows)

1. Run the installer: `AegisSetup.exe`
2. Open a new terminal and run: `Aegis --help`

## Data location

Vault data is stored at:

```
%APPDATA%\Aegis\data\passwords.txt
```

## Commands (short help)

```
help
help all
help <command>
help -e
```

## Command reference

```
help                       Show grouped help
help all                   Show full reference
help <command>             Show help for a single command
help -e                    Show example usage

add <website> <account> <pass> <note>
add -w <website> -a <account> -p <pass> [-n <note>]   Add a new entry

edit <id> <website> <account> <pass> <note>
edit <id> -w/-a/-p/-n <value>                         Edit specific fields by id

list [-p]                  List all entries (-p shows passwords)

show <id>
show -i <id>               Show a single entry by id
show -w <website>
show -a <account>
show -n <note>             Search entries (flags can be combined)

delete <id>                Delete an entry by id

copy <id>                  Copy an entry password to clipboard

gen [-l <len>] [--no-symbols]   Generate a password (prints to screen)

passwd                     Change the master password

lock                       Lock the app and re-enter the master password

clear                      Clear the screen only
clear-all                  Delete all entries (requires YES)

exit                       Exit the app
```

## Common workflows

### Add an entry

```
add site user@example.com pass123 note
add -w site -a user@example.com -p pass123 -n note
```

### Edit an entry

```
edit 2 -a user@example.com
edit 2 -w site -n note
```

### Show and search

```
show 2
show -w site
show -w site -a user
show -n note
show -n personal
```

### List

```
list
list -p
```

### Delete

```
delete 2
```

### Generate a password

```
gen

gen -l 32
gen -l 16 --no-symbols
```

### Copy password to clipboard

```
copy 2
```

### Lock and unlock

```
lock
```

### Clear the screen or wipe all entries

```
clear
clear-all
```

## Notes
- The master password is required to decrypt your vault.
- If you forget the master password, stored entries cannot be recovered.

## Development

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release
```
