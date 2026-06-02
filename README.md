# lsx | ls+
---
## ABOUT

`lsx` is a command-line tool for traversing the filesystem and editing files. It serves two main purposes:\
    1. Traversal through directories & showing their contents\
    2. Text (code) editing with syntax (basic) highlighting

## USE

`lx`

#### OPTIONS
`-sz | size     ` : show each files' disk use\
`-im | immutable` : files are immutable when viewed\
`-s | silent    ` : logging level set to CRITICAL\
`-v | verbose   ` : logging level set to DEBUG\
`-v<n>          ` : sets the logging level to n\
`/path/to/entry ` : opens the menu at /path/to entry if a directory, and opens the editor if a file

In a directory (probably not ~/, more on that later), running `lx` displays a 'menu' of all the files and folders inside your current directory. The folders are traversable (i.e., selecting a directory opens the same 'menu' from within the selected directory), and files are viewable (i.e., selecting a file opens a new window in the terminal with the file's contents). If the file's suffix (".c", ".py", etc.,) is of a known & supported language, then the rendered text will be syntactically highlighted. If it is not code or of an unknown type (".txt", ".md", etc.,), then it will be rendered as plain text (no colors).

To traverse *out of* a directory you traversed into, `p` moves you to the current directory's parent. If you have not traversed into a directory and press `p`, the UI will warn that there are none. This is because the current directory never gets its parents indexed; if it did, every time `lx` was ran, the entire disk would have to be indexed. **This is also why its probably not wise to run this in your home folder**. There are no risks to your system, but it would index the entire directory, presumably a majority of your disk size, and would be slow. Despite this, you are your own beast! Don't let me tell you what to do.

To edit a file without indexing the current directory AND w.out opening the menu, run `lx filename.txt`. The program will open directory into the file's view and exit without re-opening the menu. When run like this, lx is no more than a text editor.

In a sensitive directory, or one you want to be sure not to edit while viewing, there is also `immutable mode`. This forces the file-viewer to open in read-only, and will block edits to the file. If you attempt to edit the file in `immutable mode`, the program will warn you and stop the change. To run in `immutable mode`: `lx -im` | `lx immutable` | `lx -im filename.txt`

For more detailed information on the files, `sizes mode` shows each files' disk usage in bytes. Honestly, this feature under-performs, IMO. I left it in because it *is* useful, but just not pretty. Additionally, the directories *should* show their size (as their are indexed recursively), but does not. Maybe soon. To run in `sizes mode`: `lx -sz` | `lx -sizes`

## DEPENDENCIES

Ncurses -- terminal window UI\
ReGex   -- expression matching

## BACKGROUND

I wrote this program because I don't really like any popular CLI text-editors. Nano was my go-to, but I didn't like its UI or configuration abilities, Micro does too much, Vim is a mystery, and on and on. What self respecting programmer would get to this point *and not* write their own editor? Well, this kind, because I had no idea how. After finishing the main content from cs50's class on C, I fell in love with C and chose this for my final project. It is something I truly care about; I genuinely want/need to code to be reliable since I am currently using this program daily, and it has a broad range of components with widely varying degrees of complexity between them, as well as a wide range of things to learn.

## DESIGN CHOICES & QUIRKS

The highlighting logic is very simple, relative to modern IDEs. For each language supported, including BLANK, there is a struct for all the components of the language to highlight. Each struct (SyntaxDemands) has a field for the type of expression (i.e., function, variable, integer), a Regular Expression as a string, the same expression once compiled (expressions cached here), and the color_code, which references a RGB ColorCode. If the expression matches an instance in the code, it selects & colors it with the given type's color. Its also configured aggressively greedy; anything matched can't be matched again, so the order of the expressions is very important, and load bearing.
*The greediness is so the severity or level can be considered; an integer or string inside a comment should NOT be highlighted, so after its colored (comments run first), no other expression can color it.*
*DITTO for substitutions inside strings.*

The actual colors were originally from Shiki (shoutout) and modified. Colors are stored in RGB format with hexadecimal values, but because of Ncurses color-coding requirements (x/1000, not x/255), they have to converted. Once done, they are initiated as Ncurses custom colors in load_colors().

Indent is configured to 4 by default and never uses '\t'. TBH, I don't know why anyone would. This is actually the only thing I use Nano for ATM (Makefile requires literal Tabs). Its also much easier to track this way; with tabs, the cursor's position no longer == b->lines[curs->row].len. Instead, it would have to be checked for a '\t' char, and if found, increase the line's virtual length by '\t' - 1.

It is hardcoded to clear all lines of whitespace in the file, if written out. I hate whitespaces. Trailing spaces as the last char of a line (besides a new line) should also be trimmed, but are not ATM. BTW, if a file is written out while your cursor is in a line containing only whitespace, that line will not be cleared and your cursor will not be moved.

When 'smart' indenting, if no chars are entered before you move on the the next indented line, the empty line is cleared immediately.

`ctrl + o` | `ctrl + x` because I <3 Nano (shoutout).

Written entirely in C.

Not for or tested on Windows.

There are probably quite a few odd components to this program that I just do not see the same way. If you have any questions or suggestions, feel free to send me a message @ XX & thank you for checking out this project. I have thoroughly enjoyed making this and am proud of the result.
