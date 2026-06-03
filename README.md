# lsx | ls+

<code>lsx</code> is a command-line tool for traversing the filesystem and editing files. It has two main utilities:
- Traversal through directories & showing their contents
- Text editing with (code) syntax highlighting

<hr>

## DEPENDENCIES
<dl>
    <dt>Ncurses</dt>
    <dd>Terminal window U.I.<dd>
    <dt>ReGex</dt>
    <dd>expression matching</dd>
</dl>
<hr>

## QUICK START GUIDE

Open a menu of the current directory's contents:\
<code>./lx</code>

#### WALK THROUGH

Running <code>lx</code> opens a menu containing the current directory's files & folders as options. Move the cursor (highlighted item) between options with the direction keys.

Return to select a file for viewing or a directory for listing its contents. Folders open new menus to *their* contents, and files open a viewing window for their contents.

If the file's language type is known and supported, the contents will be syntactically highlighted. Otherwise, it will be plain text.

To traverse *up out of* a directory (i.e., <code>..</code>), <code>p</code> moves you to the current directory's parent. If you have not entered a directory, the U.I. will warn that you are at the root (the current directory never has its parent indexed).

$\textsf{\color{red} WARNING}$: Since the indexing is recursive and immediate, running <code>lx</code> in ~/, or a very large directory, will likely be painfully slow.

#### CHEAT SHEETS

###### RUNTIME ARGS

<table>
    <thead>
        <tr>
            <th>
                Short
            </th>
            <th>
                Long
            </th>
            <th>
                Effect or Result
            </th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td>
                -sz
            </td>
            <td>
                sizes
            </td> 
            <td>
                Shows disk usage of files
            </td>
        </tr>
        <tr>
            <td>
                -im
            </td>
            <td>
                immutable
            </td> 
            <td>
                Files are READ ONLY
            </td>
        </tr>
        <tr>
            <td>
                -s
            </td>
            <td>
                silent
            </td> 
            <td>
                Silence all logging
            </td>
        </tr>
        <tr>
            <td>
                -v
            </td>
            <td>
                verbose
            </td> 
            <td>
                Output all logging
            </td>
        </tr>
        <tr>
            <td colspan="2">
                -v[n]
            </td>
            <td>
                Set the logging verbosity to [n]
            </td>
        </tr>
    </tbody>
</table>

###### INTERACTING w.the U.I.

<table>
    <thead>
        <tr>
            <th>
                Key Binding
            </th>
            <th>
                Action
            </th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td>
                Enter/Return
            </td>
            <td>
                'Selects' the entry on the Menu
            </td>
        </tr>
        <tr>
            <td>
                e
            </td>
            <td>
                Edit a file (if mutable)
            </td>
        </tr>
        <tr>
            <td>
                r
            </td>
            <td>
                Read a file (edits blocked)
            </td>
        </tr>
        <tr>
            <td>
                p
            </td>
            <td>
                Edit a file (if mutable)
            </td>
        </tr>
        <tr>
            <td>
                x
            </td>
            <td>
                Exit the Menu
            </td>
        </tr>
    </tbody>
</table>

#### MORE

In a sensitive directory, or one you want to be sure not to edit while viewing, there is <code>immutable mode</code>. This forces the files opened to be in a read-only state and will block edits to the file. If you attempt to edit the file in <code>immutable mode</code>, the program will warn you and stop the change.

For more detailed information on the files, <code>sizes mode</code> shows each file's disk usage in bytes. Not my favorite feature, but I left it in because it *is* useful.

To open a file or folder other than the C.W.D., <code>lx</code> accepts a path as the last argument if more than one is passed.

## DESIGN CHOICES & QUIRKS

Aspects of this program that you, or I, might not expect.

### HIGHLIGHTING

Highlighting in this program is computed from matches found of preset RegEx expressions. There are several nuances to the flow, but basically, each 'type' to be highlighted (i.e., functions, headers, comments) has a specific RegEx expression & color code. If a match is found, it gets that color. Colors are coded to specific types and can therefore be easily adjusted without depending on its RegEx expression to identify its type.

#### The Order of Expressions Is Imperative!

The sets of <code>SyntaxDemands</code> (structs holding each regular expression, type, and color) are ordered carefully to allow enforcement of precedence for matched characters or text. Their type is set in an enum, & an error is raised if they are out of order.

In practice, this means nothing gets colored if it's already colored. The earlier an expression is, the higher its priority.

$\textsf{\color{gray} // comment w.a "string"}$

**Example**: A comment in the code contains a string. The comment's expression runs first, coloring the whole comment. Then the string's expression is run, but since its match is already colored, there is nothing for it to color. If this were not the case, the commented-out string would be rendered:

$\textsf{\color{gray} // comment w.a \ } \textsf{\color{yellow} "(incorrectly colored) string"}$

**Subtlety**: A substitution inside a string is an expression run before the string's expression, which allows the inner substitution to be detected & colored before its enclosing string is highlighted. So its mechanics are the same as above but inverted.

#### GLOBAL REGEX CACHE

When a file is being loaded with a known & supported language, the program will compile & cache the regular expressions. This lets the caller (<code>pretty_runner()</code>) manage the compiled expressions' memory & life-cycle. The compiled expressions, and count thereof, are globally accessible once compiled, allowing the editor to grab whatever is currently accessible when it is run. This means:\
- A: A language of one file won't be used to render the next one opened. 
- B: If no files are opened, no expressions are compiled.

#### BLANK

This is why the <code>BLANK</code> language is necessary. It replaces the need for a check in the editor to see *if* anything was compiled. <code>BLANK</code> is simply a set of empty expressions.

#### MULTI-LINE EXPRESSIONS

To highlight multi-line expressions (i.e., <code>/* in C */</code> or <code>"""in Python"""</code>), a different strategy was needed. Each multi-line expression gets two RegEx expressions: an initiator and a terminator. <code>walk_explicit_express()</code> walks each line of the buffer, checking each for a match to any of its initiator expressions (also ordered, for the same reason as above).

If an expression is matched, then a 'span' is begun. The function continues to walk the buffer, looking for a terminator, and consuming characters if it finds no matches. If one is found, color until that match, end the span, and restart from the current position in the buffer, looking for the first initiator. If a terminator was not found, color the line & move to the next.

### FILE SYSTEM INDEXING

When the program is run in <code>MENU_MODE</code>, one of the most important preprocessing steps done is indexing the file system. To record what is found, a file system tree of <code>FSNode</code>s is made. A single <code>FSNode</code> contains the following metadata about a single file system entry in the current directory:

<table>
    <thead>
        <tr>
            <th>
                Field Name
            </th>
            <th>
                Purpose/Utility
            </th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td>
                is_dir
            </td>
            <td>
                int; answers: is a directory ? 1 : 0;
            </td>
        </tr>
        <tr>
            <td>
                blocks
            </td>
            <td>
                long; disk size by blocks used (if file)
            </td>
        </tr>
        <tr>
            <td>
                n_children
            </td>
            <td>
                int; no. of files within (if dir)
            </td>
        </tr>
        <tr>
            <td>
                n_dirs
            </td>
            <td>
                int; no. of dirs held within (if dir)
            </td>
        </tr>
        <tr>
            <td>
                name[]
            </td>
            <td>
                char; name of entry (used to build path)
            </td>
        </tr>
        <tr>
            <td>
                **children
            </td>
            <td>
                FSNode; pointer to array of this entry's entries
            </td>
        </tr>
        <tr>
            <td>
                *parent;
            </td>
            <td>
                FSNode; pointer to the entry's parent FSnode
            </td>
        </tr>
    </tbody>
</table>

Since the only <code>FSNode</code> that contains its own full path is the root, all other paths needed have to be built from a series of concatenations. <code>untraverse()</code> is a recursive helper that builds a node's path, making it trivial in practice, but it should be noted that except for the root, no node holds its full path.

Less conveniently, this also means that the entire tree depends on the root node. Without it, or if it were lost/freed before the subsequent nodes, the entire tree would be leaked. There is only one entry point to the tree, and it must be carefully guarded, hence <code>MGMT</code>'s two <code>FSNode</code> fields. One holds the 'master' root while the other holds the currently selected entry (file or folder). When finished, <code>free_rfs()</code> takes the root and recursively frees the entire tree along with its children.

When a selection is made, menu.c populates <code>MGMT</code>'s <code>cd</code> field with the selection. If a directory, the menu is rerun with the selected directory, showing its contents the same way as the original. If the selection was a file, the editor is opened with the active mutability state, and on return/exit, <code>menu_runner()</code> sets <code>MGMT</code>'s <code>cd</code> field *to the file's parent*, so when the editor is exited, the menu is reopened with the directory from which you came.

### LOGGING OFF BY ONE

I have to admit, this one is weird, but I enjoy it. The <code>err_lvl</code> enum is indexed to the values 0-4 (0 = <code>DEBUG</code>, 4 = <code>CRITICAL</code>), but <code>print_err()</code> is called with levels 1-5. The rationale behind this error-prone architecture is clarity at call sites & a fully silenced verbosity option. When <code>print_err()</code> is called with a level, the <code>precurse()</code> function writes the error level enumerated & escaped to the appropriate color. Then, when in <code>flush_logs()</code>, the function checks to see which, if any, errors were printed out at a level >= the verbosity.

**This is how a fully-silenced option is possible.** If <code>flush_logs()</code> is run with verbosity of 5, no logging level can equal it, and therefore none are shown. Quirky, but convenient.

### MENU'S VIRTUAL GRID

To get the Menu's entries listed aesthetically, <code>order_rfs()</code> recursively sorts each FSNode to put their directories before their files. From there, the longest filename is recorded and used to set the minimum column width of the menu's grid, which determines the number of columns to make. The number of directory-only rows is found by comparing the current directory count to the column count.

**At this point, there is a (potentially only partially filled) series of rows containing only directories**.

To distinctly show the two types of entries, the files are listed **on the next empty line**. This makes a 'virtual grid' of the menu's entries which, crucially, does not represent all real values: multiple 'entries' could be non-existent if relying solely on the virtual grid.

**Example**: If there are 5 directories and 4 columns, the directories' row will span 2 rows, and 3 of the second row's 'entries' will not have real values.

To compensate for these values, the cursor's selection on-screen has to be 'skipped' over non-existent values to ensure they can't be highlighted or selected. To do so, there are some simple conditionals:
- Is the choice less than the number of directories?
    - YES: choice = choice
    - NO: choice  = first_file_row + (choice - num_of_dirs)

If the selection is NOT within the valid directories, it needs to be remapped. To skip it, the index is adjusted to count from the first file row *as if there were no missing directories*, resulting in the first file row + the choice offset by the real number of directories.

### PAD & BUFFER GROWTH

The pad's sizing strategy, and the Buffer's, is not to find out how much is needed and allocate only that amount, but rather to check if the current amount is adequate, and if not, double it. This results in fewer reallocation calls than granular strategies.

Neither the pad nor the Buffer is ever shrunk; they only ever get allocated *more* room. If a char is deleted or the number of lines is reduced, neither one's memory is worth attempting to shrink. Both get generously sized and called often to grow, because too little memory is much more of a problem than too much.

### SMART INDENTING

This was the first 'feature' I made that made it feel like a text editor instead of a buffer-editor. The logic is straightforward: if the user 'Returns' on a line that ends with a character matching one of a preset list of characters, their cursor should be indented on the newline.

Contrarily, if a user types a character that matches a character of a *different* preset list, their cursor is de-dented *on the same line* ***before the character is written***.

*Additionally, if the cursor's column != indent_level * indent_width, the indent is repaired, or attempted to be, by forcing it to the nearest valid indent column & position.*

### EMPTY LINES

Empty lines created while 'smart' indenting get truncated, as well as empty lines when the buffer is written out (the file's saved). It's a pretty aggressive behavior and definitely should be noted.

Example of 'smart' indent empty lines truncated:

User types `{` and Returns:
```
{
    | <- cursor lands here, indented by 4 spaces
```
Return is immediately entered again, and the cursor's row increases, but the indent does not change:
```
{
((this line has been cleared of spaces))
    | <- cursor is now here
```
The previously indented line is now empty, and navigating to it will drop the cursor to column 0. Subsequent returns will repeat the same behavior.

### TABS

Literal tabs (<code>'\t'</code>) are never used for indents, ever, and having them in the file would likely make the editor show incorrect or corrupted file contents.\
**VERIFIED**: when editing a Makefile that uses <code>\t</code>, the spacing was actually correct, but deleting the characters that make up the <code>\t</code> corrupted the line and likely did not do what the editor was showing.

<hr>

## BACKGROUND

I wrote this program because I don't enjoy using any popular C.L.I. text editors. Nano was my go-to, but I didn't love its U.I. or configuration options; Micro does too much; Vim is a whole thing, and testing/learning a new one sounded dreadful. What self-respecting programmer would get to this point and *not* write their own editor? Well, this kind, because I had no idea how.

After finishing the main content from CS50's Introduction to C, I fell in love with the language and chose this for my final project. The components required to build this program varied widely in difficulty and subject, so I do feel like I have covered a wide variety of applications & problems with C. Among them are communications with the underlying OS (POSIX only), interacting with the filesystem, Regular Expressions (POSIX-Compliant RegEx, which I found strange), Ncurses basics, & memory management, & a lot more.

I was challenged to think carefully about how complex systems should work together (the editor and buffer are probably the most complex) and was forced to design very intentionally; if I did not know exactly what I was attempting to do, I would inevitably write buggy and inefficient code. Being intentional and iterative in the building process allowed me to go beyond my previous scope and create something I am genuinely proud of and confident in.

*Personal Note: In my experience, my abilities have been practically limited by Python; I was never confident in its ability to be fast enough to try something like this. C, however, I have trouble making it work hard enough to register when it's working overtime. It is so gosh darn fast; I felt like ten lightbulbs went off in my head, all at once. C is fast because it does nothing for you! Granular control is always my favorite aspect of software, and Python did not make me feel that way. Everything I did bigger than a simple script had the 800-pound gorilla staring at me; if it isn't stupid efficient, it will be slow. The difference is that Python is intuitive and easier to loop & wrap up in itself. If I make something happen in Python, it just uses C to do it, but it **doesn't ask me how I would like to do it**. Obviously, this is intentional and the point of Python, but it's also why I'll probably never use it again. Middlemen are not cool.*
