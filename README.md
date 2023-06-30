# kilo

The editor from the ['Build your own text editor in C'](https://viewsourcecode.org/snaptoken/kilo/index.html) series,
but re-written in C++.

## Setup

1. Clone this repository locally

```bash
git clone git@github.com:nrednav/kilo.git
```

2. Build the project

```bash
cd kilo
make
```

3. Run the editor

```bash
# Edit a new file
./kilo

# Edit an existing file
./kilo <insert-filename>
```

## Usage

- Edit
  - To move the cursor, use the arrow keys
  - To insert a new line, use `Enter`
  - To delete a character, use `Backspace`, `Delete` or `Ctrl + h`
  - To jump to the end of a line, use the `End` key
  - To jump to the beginning of a line, use the `Home` key
  - To scroll the file down or up by an entire page length, use the `PageDown`
    or `PageUp` key
- Search
  - To initiate a search prompt, use `Ctrl + f`
  - Type the word you're looking for and hit `Enter` to perform the search
  - Alternatively, to cancel the search use `Esc`
  - To cycle through the search results:
    - Jump to next occurence = `Ctrl + n`
    - Jump to previous occurence = `Ctrl + p`
- Save
  - To save any changes you make to a file, use `Ctrl + s`
  - You will be prompted to enter a filename if you did not open a file at the
    start
  - To cancel saving, use `Esc`
- Quit
  - To quit the editor, use `Ctrl + q`
  - Then hit `y` or `n` to confirm or cancel respectively
