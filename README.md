# dswin - A command line utility for faster switching among directories in Windows

# Installation

1. Compile `main.cpp` to `dswin.exe`
2. Copy files `dswin.exe`, `ds.cmd`, `tags` to the user profile directory (`%USERPROFILE% = C:\Users\<username>`)
3. Add `C:\Users\<username>` to `PATH`.

# Using

In order to obtain a set of directory tags, edit the `%USERPROFILE%\tags` file:
```
home %USERPROFILE%
down %USERPROFILE%\Downloads
docs %USERPROFILE%\Documents
root C:\
```
Now, in order to switch to, say, `Downloads` directory, you should type in the command line `ds down`.
Addditonally, if the given tag name is not in the `tags` file, `ds` will match the closest tag by Levenshtein distance.

Finally, by typing `ds` without any arguments, you will switch to the previous directory. That way, you can jump between two directories only by typing `ds` in the command line.
