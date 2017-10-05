# How Arrow Keys Work in Xv6

- An arrow key is pressed
- The xv6 console does not have any special handling for arrow keys (they are considered "regluar" characters)
- The character is "printed" to the console
- Arrow keys show up as escape sequences, which are effectively invisible
- The terminal emulator interprets the escape sequence as a "move the cursor" command
- xv6 never thinks the cursor has moved

This has wider implications than just arrow keys. Additional escape sequences
can be passed through to your local terminal emulator just fine. This is not
particularly exciting; all that's happening is xv6 is treating the "special"
characters as though they were normal, and your local terminal emulator is doing
all the heavy lifting.
