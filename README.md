# Library Management System (C, Console Application)

A menu-driven **Library Management System** written in standard C (C11) for the
**Programming Fundamentals (CT-175)** Complex Computing Activity (CCA), Fall 2026.
It manages a catalog of books, library members, book issue and return, reservations,
late fines, searching, sorting and simple reports. All data is saved to binary files,
so nothing is lost when the program is closed.

> **Instructor:** Ms. Maham Ashraf  |  **Course:** Programming Fundamentals (CT-175)
> **GitHub repository:** https://github.com/muhammadanasneonspace/Library-management-system.c/upload/main

## Group Members

| S. No. | Student Name | Roll Number | Section | Work done |
|---|---|---|---|---|
| 1 | __MUHAMMAD ANAS __________ | ___CT-266224_________ | __E__ | Book catalog, admin menu, dynamic storage, file handling |
| 2 | __AHMED MUJTABA SHEIKH________ | ___CT-26249_________ | _E___ | Members, issue/return, reservations, borrowing limit, fines |
| 3 | __ABDUL RAFAY__________ | __CT-26244_________ | ___E_ | Search by ID/title, sorting, search reports |

## Problem Statement

A small library keeps its book records and member records on paper, which makes it slow
to search, easy to make mistakes in fine calculation, and hard to see which books are
issued. This program lets a librarian keep these records on a computer and automates
issuing, returning and fine calculation.

## Features

| No. | Feature | Description |
|---|---|---|
| 1 | Add book | Adds a book with ID, title, author, ISBN, category and copies. Rejects duplicate IDs and bad numbers. |
| 2 | Display all books | Shows a formatted table of every book. |
| 3 | Search book | Search by book ID or by exact title. |
| 4 | Update book | Changes the details of an existing book. |
| 5 | Delete book | Removes a book, but not while it is issued or reserved. |
| 6 | Sort books | Sort by ID, by title (A to Z) or by popularity (most borrowed first). |
| 7 | Reports | Book search report (ID and title list) and total number of book records. |
| 8 | Register member | Adds a member with a unique ID; shows and searches members. |
| 9 | Issue book | 14-day loan, maximum 5 books per member, checks copies and reservations. |
| 10 | Return book and fine | Calculates the late fine by book category (see below) and updates the member's balance. |
| 11 | Reserve / cancel reservation | Reserve a book that has no copies left; cancel a reservation. |
| 12 | Fine management | Show outstanding fines (total and average), sort members by fine, pay a fine, fines assessed by category. |
| 13 | Save and load | Records are stored in `books.dat` and `members.dat` and loaded at start-up. Corrupted files are detected. |
| 14 | Input validation | Every number and text input is checked; wrong input shows a message and asks again. |

### Fine rules

| Category | Rate per late day (days 1 to 7) | Rate per late day (day 8 onwards) |
|---|---|---|
| Textbook | Rs 5 | Rs 10 |
| Novel | Rs 3 | Rs 6 |
| Reference | Rs 10 | Rs 20 |

Example: a Textbook returned 10 days late costs 7 x 5 + 3 x 10 = **Rs 65.00**.

## Programming Fundamentals Concepts Used

| Concept | Where it is used |
|---|---|
| Variables and data types | `int`, `double`, `char` arrays, `size_t`, `long` for IDs, copies, fines and input parsing |
| Conditional statements | `if / else if / else` for validation and rules; `switch` for every menu and for category names |
| Loops | `do-while` menus, `for` searches, nested `for` bubble sorts, `for(;;)` re-ask input, fine loop over late days |
| Functions | Small reusable functions such as `read_int`, `find_book_index`, `calculate_fine`, `issue_book` |
| Arrays and strings | Title/author/ISBN strings, issued-books array per member, `strcmp`, `strcpy`, `fgets`, `snprintf` |
| Structures | `Book`, `Member`, `IssuedBook` and file header structs, created with `typedef` |
| Pointers and dynamic memory | `malloc`, `realloc`, `free` for the book and member arrays; pointers passed to functions |
| File handling | Binary `fopen`, `fread`, `fwrite`, `fclose`; temporary file plus `rename` for safe saving |
| Searching and sorting | Linear search and bubble sort implemented by hand |

## How to Compile and Run

You need a C compiler such as GCC (Linux, macOS, or MinGW / MSYS2 on Windows).

```bash
gcc -std=c11 -Wall -Wextra -pedantic Library_Management_System.c -o library
./library          # on Windows:  library.exe
```

The code compiles with **no warnings**. `books.dat` and `members.dat` are created
automatically in the folder you run the program from.

## How to Use (Quick Guide)

1. Start the program. The **Main Menu** appears:
   `1` Admin / Book Catalog, `2` Members / Issue / Return / Fines, `3` Display all books,
   `4` Save all records now, `0` Save and exit.
2. Open **Admin** and add some books (option `1`).
3. Open **Members** and register a member (option `1`).
4. Issue a book (option `4`): enter member ID, book ID and the current day number
   (a plain whole number such as 1, 2, 3...). The due day is shown (issue day + 14).
5. Return the book (option `5`) with the actual return day. Any late fine is added to the member.
6. Use `0` to go back and `0` again on the main menu to save and exit.

## Project Structure

```
Library-Management-System/
├── Library_Management_System.c   Source code
├── README.md                     This file
├── Project_Report.pdf            Project report
├── Project_Report.docx           Editable version of the report
├── Flowchart.pdf                 All flowcharts (group's flowchart + 8 detailed charts)
├── flowchart/                    Flowchart images (A = group's own flowchart, 1-8 detailed)
├── screenshots/                  21 screenshots of real program runs
├── test_logs/                    Full text logs of the five test runs
└── .gitignore
```

## Testing

33 test cases were run on the real program (valid input, invalid input, boundary values,
menu choices and file handling). **All 33 passed.** The full table is in
`Project_Report.pdf` (Section 7). Complete terminal logs are in `test_logs/`.
The program was also run under GCC AddressSanitizer and UndefinedBehaviorSanitizer with the
same inputs and no memory or undefined-behavior errors were reported.

<details><summary>Test case summary</summary>

| Test Case | Description | Input | Expected Output | Result |
|---|---|---|---|---|
| TC-01 | [Valid] Add a valid book | ID=101, Title=C Programming, Author=Dennis Ritchie, ISBN=9780131103627, Category=1, Copies=3 | Book added successfully. | Pass (Screenshot 2) |
| TC-02 | [Invalid] Letters and a negative number as Book ID | Book ID = abc, then -5 | Invalid input message, ask again | Pass (Screenshot 3) |
| TC-03 | [Invalid] Duplicate Book ID | Book ID = 101 (already exists) | Error: a book with this ID already exists | Pass (Screenshot 3) |
| TC-04 | [Invalid] Category outside 1 to 3 | Category = 5, then 7, then 2 | Error and ask again until a valid category | Pass (Screenshot 4) |
| TC-05 | [Boundary] Smallest copies value (0) | ID=103, Reference book, Copies = 0 | Book accepted and shown with 0 copies | Pass (Screenshot 5) |
| TC-06 | [Valid] Display all books | Admin menu option 2 | Table of all books with ID, title, author, ISBN, category, copies | Pass (Screenshot 5) |
| TC-07 | [Valid / Invalid] Search book by ID (found, not found) | ID = 102, then ID = 999 | Book details; then "Book not found." | Pass (Screenshot 6) |
| TC-08 | [Valid / Invalid] Search book by title (found, not found) | Title = The Alchemist, then No Such Title | Book details; then "No book with that title was found." | Pass (Screenshot 6) |
| TC-09 | [Valid] Update an existing book | ID=103, new title Oxford English Dictionary, copies = 2 | Book updated successfully. | Pass (Screenshot 7) |
| TC-10 | [Valid / Invalid] Delete a book (existing, missing) | ID = 199, then ID = 999 | Book deleted successfully; then "Book not found." | Pass (Screenshot 7) |
| TC-11 | [Valid] Sort books alphabetically by title | Admin menu option 8 | Books listed A to Z (C Programming, Data Structures, ...) | Pass (Screenshot 8) |
| TC-12 | [Valid] Sort books by popularity (after borrowing) | Admin menu option 9 after restart | Most borrowed book first | Pass (Screenshot 20) |
| TC-13 | [Valid] Book search report and total records | Admin menu options 10 and 11 | Total records = 6 and a list of ID and title | Pass (Screenshot 8) |
| TC-14 | [Menu choice] Option outside the menu / letters | Admin menu: 12 and x; main menu: 9 and abc | Invalid input message, menu shown again | Pass (Screenshot 9) |
| TC-15 | [Valid] Register a member | ID=1, Name = Ali Raza | Member registered successfully. | Pass (Screenshot 10) |
| TC-16 | [Invalid] Duplicate member ID | Member ID = 1 (already exists) | Error: a member with this ID already exists | Pass (Screenshot 10) |
| TC-17 | [Valid / Invalid] Search member (found, not found) | Member ID = 1, then 77 | Member details; then "Member not found." | Pass (Screenshot 10) |
| TC-18 | [Valid] Issue a book | Member 1, Book 101, day 1 | Issued successfully, due day = 1 + 14 = 15 | Pass (Screenshot 11) |
| TC-19 | [Invalid] Issue: unknown member / no copies left | Member 99; Member 2 with Book 102 (0 copies) | Member not found; No copies available | Pass (Screenshot 11) |
| TC-20 | [Valid / Invalid] Reserve unavailable book | Member 2 reserves Book 102 twice; member 3 reserves it; member 2 reserves Book 101 (copies available) | Reserved; already reserved (twice); copies available | Pass (Screenshot 12) |
| TC-21 | [Valid / Invalid] Cancel reservation | Member 3 (wrong), then member 2 for Book 102 | Not reserved by that member; then cancelled | Pass (Screenshot 12) |
| TC-22 | [Valid] Return a Textbook 10 days late | Member 1, Book 101, return day 25 (due 15) | Fine = 7 x 5 + 3 x 10 = 65.00 | Pass (Screenshot 13) |
| TC-23 | [Invalid] Return day before the issue day | Member 2, Book 104 (issued day 5), return day 2, then 30 | Error and ask again; then fine 7 x 5 + 4 x 10 = 75.00 | Pass (Screenshot 13) |
| TC-24 | [Boundary] Return exactly on the due day, and 1 day late | Member 2, Book 105: due 64, return 64; due 84, return 85 | Fine 0.00 on the due day; 5.00 when 1 day late | Pass (Screenshot 18) |
| TC-25 | [Invalid] Return a book the member does not hold | Member 1, Book 103 | Error: member does not have that book | Pass (Screenshot 13) |
| TC-26 | [Boundary] Borrowing limit of 5 books | Member 3 already holds 5 books, tries to issue a 6th | Borrowing limit reached | Pass (Screenshot 14) |
| TC-27 | [Valid] Show outstanding fines | Member menu option 8 | Total 155.00 and average 51.67 | Pass (Screenshot 15) |
| TC-28 | [Valid] Sort members by fine (highest first) | Member menu option 9 | Ali Raza (80.00) before Sara Khan (75.00) | Pass (Screenshot 15) |
| TC-29 | [Invalid / Valid] Pay fine: text, too much, then valid | Member 1 (fine 80.00): abc, 500, then 30 | Two errors, then remaining fine 50.00 | Pass (Screenshot 15) |
| TC-30 | [Valid] Fines assessed by category | Member menu option 11 | Textbook 140.00, Novel 15.00, Reference 0.00, Total 155.00 | Pass (Screenshot 16) |
| TC-31 | [Invalid] Delete a book that is currently issued | Admin option 6, Book ID = 101 (held by member 3) | Error: cannot be deleted while issued | Pass (Screenshot 17) |
| TC-32 | [File] Data is still there after restart | Close the program, run it again | Loaded 6 books and 3 members | Pass (Screenshot 19) |
| TC-33 | [File] Corrupted books.dat file | books.dat replaced by random text | Warning, empty catalog, no crash | Pass (Screenshot 21) |

</details>

## Known Limitations and Future Improvements

- Book search by title needs the complete exact title.
- The "day number" is typed by the user instead of read from a calendar date.
- There is no login for the librarian; adding a password would improve security.
- Data is stored in binary files; moving to a database would support larger libraries.

