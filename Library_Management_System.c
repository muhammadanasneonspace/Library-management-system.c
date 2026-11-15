/*
 * LIBRARY MANAGEMENT SYSTEM - COMBINED GROUP PROJECT
 * Person 1: Book catalog, admin menu, dynamic storage, file handling
 * Person 2: Members, issue/return, reservations, borrowing limits, fines
 * Person 3: Search by ID/title, sorting, and search reports
 *
 * Compile: gcc -std=c11 -Wall -Wextra -pedantic Library_Management_System.c -o library
 * Run:     ./library
 * Data files (created automatically): books.dat and members.dat
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define TITLE_LEN 100
#define AUTHOR_LEN 100
#define ISBN_LEN 24
#define CATEGORY_LEN 20
#define NAME_LEN 80
#define MAX_ISSUED 5
#define LOAN_DAYS 14
#define MAX_RECORDS 10000
#define BOOK_FILE "books.dat"
#define MEMBER_FILE "members.dat"
#define BOOK_MAGIC "LIBBOOK1"
#define MEMBER_MAGIC "LIBMEM01"
#define FILE_VERSION 1

typedef struct {
    int id;
    char title[TITLE_LEN];
    char author[AUTHOR_LEN];
    char isbn[ISBN_LEN];
    int category;              /* 1 = Textbook, 2 = Novel, 3 = Reference */
    int copies;                /* copies currently available */
    int reserved_by;           /* member ID, or 0 if not reserved */
    int times_borrowed;
} Book;

typedef struct {
    int book_id;
    int issue_day;
    int due_day;
} IssuedBook;

typedef struct {
    int id;
    char name[NAME_LEN];
    double fine;
    int issued_count;
    IssuedBook issued[MAX_ISSUED];
} Member;

typedef struct {
    char magic[8];
    int version;
    int count;
} BookFileHeader;

typedef struct {
    char magic[8];
    int version;
    int count;
    double fine_by_category[3];
} MemberFileHeader;

static Book *books = NULL;
static Member *members = NULL;
static int book_count = 0;
static int member_count = 0;
static int book_capacity = 0;
static int member_capacity = 0;
static double fine_by_category[3] = {0.0, 0.0, 0.0};

/* ------------------------- INPUT HELPERS ------------------------- */
static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}

static int read_line(const char *prompt, char *buffer, size_t size) {
    int ch;
    size_t len;
    if (size == 0) return 0;
    printf("%s", prompt);
    if (fgets(buffer, (int)size, stdin) == NULL) return 0;
    len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else {
        while ((ch = getchar()) != '\n' && ch != EOF) { }
    }
    trim_newline(buffer);
    return 1;
}

static int read_text(const char *prompt, char *buffer, size_t size) {
    for (;;) {
        if (!read_line(prompt, buffer, size)) return 0;
        if (buffer[0] == '\0') {
            printf("Input cannot be empty. Please try again.\n");
            continue;
        }
        return 1;
    }
}

static int read_int(const char *prompt, int min_value, int max_value, int *value) {
    char line[128];
    char *end;
    long parsed;
    for (;;) {
        if (!read_line(prompt, line, sizeof line)) return 0;
        errno = 0;
        parsed = strtol(line, &end, 10);
        while (*end == ' ' || *end == '\t') end++;
        if (line[0] != '\0' && *end == '\0' && errno != ERANGE &&
            parsed >= min_value && parsed <= max_value) {
            *value = (int)parsed;
            return 1;
        }
        printf("Invalid input. Enter a whole number from %d to %d.\n",
               min_value, max_value);
    }
}

static int read_money(const char *prompt, double min_value, double max_value,
                      double *value) {
    char line[128];
    char *end;
    double parsed;
    for (;;) {
        if (!read_line(prompt, line, sizeof line)) return 0;
        errno = 0;
        parsed = strtod(line, &end);
        while (*end == ' ' || *end == '\t') end++;
        if (line[0] != '\0' && *end == '\0' && errno != ERANGE &&
            parsed >= min_value && parsed <= max_value) {
            *value = parsed;
            return 1;
        }
        printf("Invalid amount. Enter a value from %.2f to %.2f.\n",
               min_value, max_value);
    }
}

static int valid_category(int category) {
    return category >= 1 && category <= 3;
}

static const char *category_name(int category) {
    switch (category) {
        case 1: return "Textbook";
        case 2: return "Novel";
        case 3: return "Reference";
        default: return "Unknown";
    }
}

/* ------------------------- DYNAMIC ARRAYS ------------------------- */
static int ensure_book_capacity(void) {
    Book *temp;
    int new_capacity;
    if (book_count < book_capacity) return 1;
    if (book_capacity >= MAX_RECORDS) {
        printf("Book storage limit reached.\n");
        return 0;
    }
    new_capacity = (book_capacity == 0) ? 10 : book_capacity * 2;
    if (new_capacity > MAX_RECORDS) new_capacity = MAX_RECORDS;
    temp = (Book *)realloc(books, (size_t)new_capacity * sizeof(Book));
    if (temp == NULL) {
        printf("Memory allocation failed. Book was not added.\n");
        return 0;
    }
    books = temp;
    book_capacity = new_capacity;
    return 1;
}

static int ensure_member_capacity(void) {
    Member *temp;
    int new_capacity;
    if (member_count < member_capacity) return 1;
    if (member_capacity >= MAX_RECORDS) {
        printf("Member storage limit reached.\n");
        return 0;
    }
    new_capacity = (member_capacity == 0) ? 10 : member_capacity * 2;
    if (new_capacity > MAX_RECORDS) new_capacity = MAX_RECORDS;
    temp = (Member *)realloc(members, (size_t)new_capacity * sizeof(Member));
    if (temp == NULL) {
        printf("Memory allocation failed. Member was not registered.\n");
        return 0;
    }
    members = temp;
    member_capacity = new_capacity;
    return 1;
}

/* ------------------------- LOOKUP HELPERS ------------------------- */
static int find_book_index(int id) {
    int i;
    for (i = 0; i < book_count; i++) {
        if (books[i].id == id) return i;
    }
    return -1;
}

static int find_member_index(int id) {
    int i;
    for (i = 0; i < member_count; i++) {
        if (members[i].id == id) return i;
    }
    return -1;
}

static int book_is_issued(int book_id) {
    int i, j;
    for (i = 0; i < member_count; i++) {
        for (j = 0; j < members[i].issued_count; j++) {
            if (members[i].issued[j].book_id == book_id) return 1;
        }
    }
    return 0;
}

/* ------------------------- FILE HANDLING ------------------------- */
static int save_books(void) {
    FILE *fp;
    BookFileHeader header;
    char temp_name[64];
    (void)snprintf(temp_name, sizeof temp_name, "%s.tmp", BOOK_FILE);
    fp = fopen(temp_name, "wb");
    if (fp == NULL) {
        printf("Could not open temporary book file for saving.\n");
        return 0;
    }
    memcpy(header.magic, BOOK_MAGIC, 8);
    header.version = FILE_VERSION;
    header.count = book_count;
    if (fwrite(&header, sizeof header, 1, fp) != 1 ||
        (book_count > 0 && fwrite(books, sizeof(Book), (size_t)book_count, fp) != (size_t)book_count)) {
        printf("Error writing book data.\n");
        fclose(fp);
        remove(temp_name);
        return 0;
    }
    if (fclose(fp) != 0) {
        printf("Error closing book data file.\n");
        remove(temp_name);
        return 0;
    }
    if (rename(temp_name, BOOK_FILE) != 0) {
        remove(BOOK_FILE);
        if (rename(temp_name, BOOK_FILE) != 0) {
            printf("Could not replace book data file.\n");
            remove(temp_name);
            return 0;
        }
    }
    return 1;
}

static int save_members(void) {
    FILE *fp;
    MemberFileHeader header;
    char temp_name[64];
    (void)snprintf(temp_name, sizeof temp_name, "%s.tmp", MEMBER_FILE);
    fp = fopen(temp_name, "wb");
    if (fp == NULL) {
        printf("Could not open temporary member file for saving.\n");
        return 0;
    }
    memcpy(header.magic, MEMBER_MAGIC, 8);
    header.version = FILE_VERSION;
    header.count = member_count;
    memcpy(header.fine_by_category, fine_by_category, sizeof fine_by_category);
    if (fwrite(&header, sizeof header, 1, fp) != 1 ||
        (member_count > 0 && fwrite(members, sizeof(Member), (size_t)member_count, fp) != (size_t)member_count)) {
        printf("Error writing member data.\n");
        fclose(fp);
        remove(temp_name);
        return 0;
    }
    if (fclose(fp) != 0) {
        printf("Error closing member data file.\n");
        remove(temp_name);
        return 0;
    }
    if (rename(temp_name, MEMBER_FILE) != 0) {
        remove(MEMBER_FILE);
        if (rename(temp_name, MEMBER_FILE) != 0) {
            printf("Could not replace member data file.\n");
            remove(temp_name);
            return 0;
        }
    }
    return 1;
}

static int valid_book_record(const Book *b) {
    if (b->id <= 0 || !valid_category(b->category) || b->copies < 0 ||
        b->reserved_by < 0 || b->times_borrowed < 0) return 0;
    if (memchr(b->title, '\0', sizeof b->title) == NULL ||
        memchr(b->author, '\0', sizeof b->author) == NULL ||
        memchr(b->isbn, '\0', sizeof b->isbn) == NULL) return 0;
    if (b->title[0] == '\0' || b->author[0] == '\0') return 0;
    return 1;
}

static int valid_member_record(const Member *m) {
    int j;
    if (m->id <= 0 || m->fine < 0.0 || m->issued_count < 0 ||
        m->issued_count > MAX_ISSUED) return 0;
    if (memchr(m->name, '\0', sizeof m->name) == NULL || m->name[0] == '\0') return 0;
    for (j = 0; j < m->issued_count; j++) {
        if (m->issued[j].book_id <= 0 || m->issued[j].issue_day < 0 ||
            m->issued[j].due_day < m->issued[j].issue_day) return 0;
    }
    return 1;
}

static int load_books(void) {
    FILE *fp = fopen(BOOK_FILE, "rb");
    BookFileHeader header;
    Book *loaded = NULL;
    int i, ch;
    if (fp == NULL) return 1; /* First run: no file yet. */
    if (fread(&header, sizeof header, 1, fp) != 1 ||
        memcmp(header.magic, BOOK_MAGIC, 8) != 0 ||
        header.version != FILE_VERSION || header.count < 0 || header.count > MAX_RECORDS) {
        printf("Warning: books.dat is missing a valid header or is corrupted. Starting with an empty catalog.\n");
        fclose(fp);
        return 0;
    }
    if (header.count > 0) {
        loaded = (Book *)malloc((size_t)header.count * sizeof(Book));
        if (loaded == NULL) {
            printf("Not enough memory to load books.\n");
            fclose(fp);
            return 0;
        }
        if (fread(loaded, sizeof(Book), (size_t)header.count, fp) != (size_t)header.count) {
            printf("Warning: book file is incomplete. Catalog was not loaded.\n");
            free(loaded);
            fclose(fp);
            return 0;
        }
    }
    ch = fgetc(fp);
    if (ch != EOF || ferror(fp)) {
        printf("Warning: extra or unreadable data found in books.dat. Catalog was not loaded.\n");
        free(loaded);
        fclose(fp);
        return 0;
    }
    fclose(fp);
    for (i = 0; i < header.count; i++) {
        int j;
        if (!valid_book_record(&loaded[i])) {
            printf("Warning: invalid book record found. Catalog was not loaded.\n");
            free(loaded);
            return 0;
        }
        for (j = 0; j < i; j++) {
            if (loaded[j].id == loaded[i].id) {
                printf("Warning: duplicate book IDs found in file. Catalog was not loaded.\n");
                free(loaded);
                return 0;
            }
        }
    }
    books = loaded;
    book_count = header.count;
    book_capacity = header.count;
    return 1;
}

static int load_members(void) {
    FILE *fp = fopen(MEMBER_FILE, "rb");
    MemberFileHeader header;
    Member *loaded = NULL;
    int i, ch;
    if (fp == NULL) return 1;
    if (fread(&header, sizeof header, 1, fp) != 1 ||
        memcmp(header.magic, MEMBER_MAGIC, 8) != 0 ||
        header.version != FILE_VERSION || header.count < 0 || header.count > MAX_RECORDS) {
        printf("Warning: members.dat has an invalid header or is corrupted. Starting with an empty member list.\n");
        fclose(fp);
        return 0;
    }
    for (i = 0; i < 3; i++) {
        if (header.fine_by_category[i] < 0.0) {
            printf("Warning: invalid fine summary in members.dat. Member data was not loaded.\n");
            fclose(fp);
            return 0;
        }
    }
    if (header.count > 0) {
        loaded = (Member *)malloc((size_t)header.count * sizeof(Member));
        if (loaded == NULL) {
            printf("Not enough memory to load members.\n");
            fclose(fp);
            return 0;
        }
        if (fread(loaded, sizeof(Member), (size_t)header.count, fp) != (size_t)header.count) {
            printf("Warning: member file is incomplete. Member list was not loaded.\n");
            free(loaded);
            fclose(fp);
            return 0;
        }
    }
    ch = fgetc(fp);
    if (ch != EOF || ferror(fp)) {
        printf("Warning: extra or unreadable data found in members.dat. Member list was not loaded.\n");
        free(loaded);
        fclose(fp);
        return 0;
    }
    fclose(fp);
    for (i = 0; i < header.count; i++) {
        int j;
        if (!valid_member_record(&loaded[i])) {
            printf("Warning: invalid member record found. Member list was not loaded.\n");
            free(loaded);
            return 0;
        }
        for (j = 0; j < i; j++) {
            if (loaded[j].id == loaded[i].id) {
                printf("Warning: duplicate member IDs found. Member list was not loaded.\n");
                free(loaded);
                return 0;
            }
        }
    }
    members = loaded;
    member_count = header.count;
    member_capacity = header.count;
    memcpy(fine_by_category, header.fine_by_category, sizeof fine_by_category);
    return 1;
}

static void save_all(void) {
    int books_ok = save_books();
    int members_ok = save_members();
    if (books_ok && members_ok) printf("All records saved successfully.\n");
    else printf("Warning: one or more data files could not be saved.\n");
}

/* ------------------------- PERSON 1: BOOK CATALOG ------------------------- */
static void add_book(void) {
    Book b;
    int id;
    if (!read_int("Enter Book ID (positive): ", 1, INT_MAX, &id)) return;
    if (find_book_index(id) != -1) {
        printf("A book with this ID already exists.\n");
        return;
    }
    if (!read_text("Enter title: ", b.title, sizeof b.title) ||
        !read_text("Enter author: ", b.author, sizeof b.author) ||
        !read_text("Enter ISBN: ", b.isbn, sizeof b.isbn)) return;
    printf("Categories: 1. Textbook  2. Novel  3. Reference\n");
    if (!read_int("Choose category: ", 1, 3, &b.category) ||
        !read_int("Enter total available copies (0 or more): ", 0, 1000000, &b.copies)) return;
    b.id = id;
    b.reserved_by = 0;
    b.times_borrowed = 0;
    if (!ensure_book_capacity()) return;
    books[book_count++] = b;
    printf("Book added successfully.\n");
    (void)save_books();
}

static void display_books(void) {
    int i;
    if (book_count == 0) {
        printf("No books are currently registered.\n");
        return;
    }
    printf("\n%-7s %-28s %-22s %-16s %-12s %-8s %-9s\n",
           "ID", "TITLE", "AUTHOR", "ISBN", "CATEGORY", "COPIES", "BORROWED");
    printf("------------------------------------------------------------------------------------------------------\n");
    for (i = 0; i < book_count; i++) {
        printf("%-7d %-28.28s %-22.22s %-16.16s %-12s %-8d %-9d\n",
               books[i].id, books[i].title, books[i].author, books[i].isbn,
               category_name(books[i].category), books[i].copies, books[i].times_borrowed);
        if (books[i].reserved_by != 0)
            printf("        Reserved for member ID: %d\n", books[i].reserved_by);
    }
}

static void print_book_details(const Book *b) {
    printf("\nBook ID: %d\nTitle: %s\nAuthor: %s\nISBN: %s\nCategory: %s\nAvailable copies: %d\nTimes borrowed: %d\n",
           b->id, b->title, b->author, b->isbn, category_name(b->category),
           b->copies, b->times_borrowed);
    if (b->reserved_by != 0) printf("Reserved by member ID: %d\n", b->reserved_by);
}

static void search_book_by_id(void) {
    int id, index;
    if (!read_int("Enter Book ID: ", 1, INT_MAX, &id)) return;
    index = find_book_index(id);
    if (index == -1) printf("Book not found.\n");
    else print_book_details(&books[index]);
}

static void search_book_by_title(void) {
    char title[TITLE_LEN];
    int i, found = 0;
    if (!read_text("Enter complete book title: ", title, sizeof title)) return;
    for (i = 0; i < book_count; i++) {
        if (strcmp(books[i].title, title) == 0) {
            print_book_details(&books[i]);
            found = 1;
        }
    }
    if (!found) printf("No book with that title was found.\n");
}

static void update_book(void) {
    int id, index, category, copies;
    char title[TITLE_LEN], author[AUTHOR_LEN], isbn[ISBN_LEN];
    if (!read_int("Enter Book ID to update: ", 1, INT_MAX, &id)) return;
    index = find_book_index(id);
    if (index == -1) {
        printf("Book not found.\n");
        return;
    }
    if (!read_text("New title: ", title, sizeof title) ||
        !read_text("New author: ", author, sizeof author) ||
        !read_text("New ISBN: ", isbn, sizeof isbn)) return;
    printf("Categories: 1. Textbook  2. Novel  3. Reference\n");
    if (!read_int("New category: ", 1, 3, &category) ||
        !read_int("New available copies: ", 0, 1000000, &copies)) return;
    strcpy(books[index].title, title);
    strcpy(books[index].author, author);
    strcpy(books[index].isbn, isbn);
    books[index].category = category;
    books[index].copies = copies;
    printf("Book updated successfully.\n");
    (void)save_books();
}

static void delete_book(void) {
    int id, index, i;
    if (!read_int("Enter Book ID to delete: ", 1, INT_MAX, &id)) return;
    index = find_book_index(id);
    if (index == -1) {
        printf("Book not found.\n");
        return;
    }
    if (book_is_issued(id)) {
        printf("This book cannot be deleted because it is currently issued.\n");
        return;
    }
    if (books[index].reserved_by != 0) {
        printf("Cancel the reservation before deleting this book.\n");
        return;
    }
    for (i = index; i < book_count - 1; i++) books[i] = books[i + 1];
    book_count--;
    printf("Book deleted successfully.\n");
    (void)save_books();
}

static void sort_books_by_id(void) {
    int i, j;
    Book temp;
    for (i = 0; i < book_count - 1; i++) {
        for (j = 0; j < book_count - i - 1; j++) {
            if (books[j].id > books[j + 1].id) {
                temp = books[j]; books[j] = books[j + 1]; books[j + 1] = temp;
            }
        }
    }
    printf("Books sorted by ID.\n");
    display_books();
    (void)save_books();
}

static void sort_books_by_title(void) {
    int i, j;
    Book temp;
    for (i = 0; i < book_count - 1; i++) {
        for (j = 0; j < book_count - i - 1; j++) {
            if (strcmp(books[j].title, books[j + 1].title) > 0) {
                temp = books[j]; books[j] = books[j + 1]; books[j + 1] = temp;
            }
        }
    }
    printf("Books sorted alphabetically by title.\n");
    display_books();
    (void)save_books();
}

static void sort_books_by_popularity(void) {
    int i, j;
    Book temp;
    for (i = 0; i < book_count - 1; i++) {
        for (j = 0; j < book_count - i - 1; j++) {
            if (books[j].times_borrowed < books[j + 1].times_borrowed) {
                temp = books[j]; books[j] = books[j + 1]; books[j + 1] = temp;
            }
        }
    }
    printf("Books sorted by popularity (most borrowed first).\n");
    display_books();
    (void)save_books();
}

static void book_search_report(void) {
    int i;
    printf("\n========== BOOK SEARCH REPORT ==========\n");
    printf("Total book titles/records: %d\n", book_count);
    printf("\n%-10s %s\n", "BOOK ID", "BOOK TITLE");
    for (i = 0; i < book_count; i++)
        printf("%-10d %s\n", books[i].id, books[i].title);
    printf("========================================\n");
}

static void admin_menu(void) {
    int choice;
    do {
        printf("\n========== ADMIN / BOOK CATALOG MENU ==========\n");
        printf("1. Add book\n2. Display all books\n3. Search book by ID\n");
        printf("4. Search book by title\n5. Update book\n6. Delete book\n");
        printf("7. Sort by ID\n8. Sort by title\n9. Sort by popularity\n");
        printf("10. Book search report\n11. Total book records\n0. Back to main menu\n");
        if (!read_int("Enter choice: ", 0, 11, &choice)) return;
        switch (choice) {
            case 1: add_book(); break;
            case 2: display_books(); break;
            case 3: search_book_by_id(); break;
            case 4: search_book_by_title(); break;
            case 5: update_book(); break;
            case 6: delete_book(); break;
            case 7: sort_books_by_id(); break;
            case 8: sort_books_by_title(); break;
            case 9: sort_books_by_popularity(); break;
            case 10: book_search_report(); break;
            case 11: printf("Total book records: %d\n", book_count); break;
            case 0: break;
            default: printf("Invalid menu choice.\n");
        }
    } while (choice != 0);
}

/* ------------------------- PERSON 2: MEMBERS / CIRCULATION ------------------------- */
static void register_member(void) {
    Member m;
    int id;
    if (!read_int("Enter new Member ID (positive): ", 1, INT_MAX, &id)) return;
    if (find_member_index(id) != -1) {
        printf("A member with this ID already exists.\n");
        return;
    }
    if (!read_text("Enter member name: ", m.name, sizeof m.name)) return;
    m.id = id;
    m.fine = 0.0;
    m.issued_count = 0;
    memset(m.issued, 0, sizeof m.issued);
    if (!ensure_member_capacity()) return;
    members[member_count++] = m;
    printf("Member registered successfully.\n");
    (void)save_members();
}

static void display_members(void) {
    int i;
    if (member_count == 0) {
        printf("No members registered.\n");
        return;
    }
    printf("\n%-8s %-24s %-12s %s\n", "ID", "NAME", "FINE", "ISSUED");
    for (i = 0; i < member_count; i++)
        printf("%-8d %-24.24s %-12.2f %d/%d\n", members[i].id,
               members[i].name, members[i].fine, members[i].issued_count, MAX_ISSUED);
}

static void search_member(void) {
    int id, index, j;
    if (!read_int("Enter Member ID: ", 1, INT_MAX, &id)) return;
    index = find_member_index(id);
    if (index == -1) {
        printf("Member not found.\n");
        return;
    }
    printf("\nMember ID: %d\nName: %s\nFine balance: %.2f\nIssued books: %d/%d\n",
           members[index].id, members[index].name, members[index].fine,
           members[index].issued_count, MAX_ISSUED);
    for (j = 0; j < members[index].issued_count; j++)
        printf("Book ID: %d | Issued day: %d | Due day: %d\n",
               members[index].issued[j].book_id,
               members[index].issued[j].issue_day,
               members[index].issued[j].due_day);
}

static double fine_rate_for_category(int category) {
    switch (category) {
        case 1: return 5.0;
        case 2: return 3.0;
        case 3: return 10.0;
        default: return 2.0;
    }
}

static double calculate_fine(int category, int due_day, int return_day) {
    int late_days = return_day - due_day;
    int day;
    double rate = fine_rate_for_category(category);
    double fine = 0.0;
    if (late_days <= 0) return 0.0;
    for (day = 1; day <= late_days; day++) {
        if (day <= 7) fine += rate;
        else fine += rate * 2.0;
    }
    return fine;
}

static void issue_book(void) {
    int member_id, book_id, issue_day, m, b, j;
    if (!read_int("Enter Member ID: ", 1, INT_MAX, &member_id)) return;
    m = find_member_index(member_id);
    if (m == -1) { printf("Member not found. Register the member first.\n"); return; }
    if (members[m].issued_count >= MAX_ISSUED) {
        printf("Borrowing limit reached. A member may hold at most %d books.\n", MAX_ISSUED);
        return;
    }
    if (!read_int("Enter Book ID: ", 1, INT_MAX, &book_id)) return;
    b = find_book_index(book_id);
    if (b == -1) { printf("Book not found.\n"); return; }
    for (j = 0; j < members[m].issued_count; j++) {
        if (members[m].issued[j].book_id == book_id) {
            printf("This member already has this book.\n"); return;
        }
    }
    if (books[b].reserved_by != 0 && books[b].reserved_by != member_id) {
        printf("This book is reserved for another member.\n"); return;
    }
    if (books[b].copies <= 0) { printf("No copies are currently available.\n"); return; }
    if (!read_int("Enter current day number (0 or greater): ", 0, INT_MAX - LOAN_DAYS, &issue_day)) return;
    j = members[m].issued_count;
    members[m].issued[j].book_id = book_id;
    members[m].issued[j].issue_day = issue_day;
    members[m].issued[j].due_day = issue_day + LOAN_DAYS;
    members[m].issued_count++;
    books[b].copies--;
    books[b].times_borrowed++;
    if (books[b].reserved_by == member_id) books[b].reserved_by = 0;
    printf("Book issued successfully. Due day: %d\n", issue_day + LOAN_DAYS);
    save_all();
}

static void return_book(void) {
    int member_id, book_id, return_day, m, b, j, position = -1;
    double fine;
    if (!read_int("Enter Member ID: ", 1, INT_MAX, &member_id)) return;
    m = find_member_index(member_id);
    if (m == -1) { printf("Member not found.\n"); return; }
    if (!read_int("Enter Book ID to return: ", 1, INT_MAX, &book_id)) return;
    for (j = 0; j < members[m].issued_count; j++) {
        if (members[m].issued[j].book_id == book_id) { position = j; break; }
    }
    if (position == -1) { printf("This member does not have that book issued.\n"); return; }
    b = find_book_index(book_id);
    if (b == -1) { printf("Book record is missing; return cannot be completed safely.\n"); return; }
    if (!read_int("Enter actual return day: ", members[m].issued[position].issue_day, INT_MAX, &return_day)) return;
    fine = calculate_fine(books[b].category, members[m].issued[position].due_day, return_day);
    members[m].fine += fine;
    fine_by_category[books[b].category - 1] += fine;
    books[b].copies++;
    for (j = position; j < members[m].issued_count - 1; j++)
        members[m].issued[j] = members[m].issued[j + 1];
    members[m].issued_count--;
    memset(&members[m].issued[members[m].issued_count], 0, sizeof(IssuedBook));
    printf("Book returned successfully. Late fine added: %.2f\n", fine);
    printf("Member's remaining fine balance: %.2f\n", members[m].fine);
    save_all();
}

static void reserve_book(void) {
    int member_id, book_id, m, b;
    if (!read_int("Enter Member ID: ", 1, INT_MAX, &member_id)) return;
    m = find_member_index(member_id);
    if (m == -1) { printf("Member not found.\n"); return; }
    if (!read_int("Enter Book ID to reserve: ", 1, INT_MAX, &book_id)) return;
    b = find_book_index(book_id);
    if (b == -1) { printf("Book not found.\n"); return; }
    if (books[b].reserved_by == member_id) { printf("You have already reserved this book.\n"); return; }
    if (books[b].reserved_by != 0) { printf("This book is already reserved by another member.\n"); return; }
    if (books[b].copies > 0) { printf("Copies are available; please issue the book instead of reserving it.\n"); return; }
    books[b].reserved_by = member_id;
    printf("Book reserved successfully for member %d.\n", member_id);
    (void)save_books();
}

static void cancel_reservation(void) {
    int member_id, book_id, b;
    if (!read_int("Enter Member ID: ", 1, INT_MAX, &member_id)) return;
    if (find_member_index(member_id) == -1) { printf("Member not found.\n"); return; }
    if (!read_int("Enter Book ID: ", 1, INT_MAX, &book_id)) return;
    b = find_book_index(book_id);
    if (b == -1) { printf("Book not found.\n"); return; }
    if (books[b].reserved_by != member_id) {
        printf("This book is not reserved by that member.\n"); return;
    }
    books[b].reserved_by = 0;
    printf("Reservation cancelled.\n");
    (void)save_books();
}

static void show_fines(void) {
    int i;
    double total = 0.0;
    if (member_count == 0) { printf("No members registered.\n"); return; }
    printf("\n%-8s %-24s %s\n", "ID", "NAME", "FINE");
    for (i = 0; i < member_count; i++) {
        printf("%-8d %-24.24s %.2f\n", members[i].id, members[i].name, members[i].fine);
        total += members[i].fine;
    }
    printf("Total outstanding fines: %.2f\n", total);
    printf("Average outstanding fine: %.2f\n", total / member_count);
}

static void show_fines_by_category(void) {
    printf("\n===== TOTAL FINES ASSESSED BY CATEGORY =====\n");
    printf("Textbook:  %.2f\n", fine_by_category[0]);
    printf("Novel:     %.2f\n", fine_by_category[1]);
    printf("Reference: %.2f\n", fine_by_category[2]);
    printf("Total:     %.2f\n", fine_by_category[0] + fine_by_category[1] + fine_by_category[2]);
    printf("Note: This report shows fines assessed over time, not only unpaid balances.\n");
}

static void sort_members_by_fine(void) {
    int i, j;
    Member temp;
    for (i = 0; i < member_count - 1; i++) {
        for (j = 0; j < member_count - i - 1; j++) {
            if (members[j].fine < members[j + 1].fine) {
                temp = members[j]; members[j] = members[j + 1]; members[j + 1] = temp;
            }
        }
    }
    printf("Members sorted by outstanding fine (highest first).\n");
    display_members();
}

static void pay_fine(void) {
    int id, m;
    double amount;
    if (!read_int("Enter Member ID: ", 1, INT_MAX, &id)) return;
    m = find_member_index(id);
    if (m == -1) { printf("Member not found.\n"); return; }
    if (members[m].fine <= 0.0) { printf("This member has no outstanding fine.\n"); return; }
    printf("Outstanding fine: %.2f\n", members[m].fine);
    if (!read_money("Enter payment amount: ", 0.01, members[m].fine, &amount)) return;
    members[m].fine -= amount;
    if (members[m].fine < 0.005) members[m].fine = 0.0;
    printf("Payment recorded. Remaining fine: %.2f\n", members[m].fine);
    (void)save_members();
}

static void member_menu(void) {
    int choice;
    do {
        printf("\n========== MEMBER / ISSUE / RETURN MENU ==========\n");
        printf("1. Register member\n2. Display all members\n3. Search member\n");
        printf("4. Issue book\n5. Return book and calculate fine\n6. Reserve unavailable book\n");
        printf("7. Cancel reservation\n8. Show outstanding fines\n9. Sort members by fine\n");
        printf("10. Pay fine\n11. Fines assessed by category\n0. Back to main menu\n");
        if (!read_int("Enter choice: ", 0, 11, &choice)) return;
        switch (choice) {
            case 1: register_member(); break;
            case 2: display_members(); break;
            case 3: search_member(); break;
            case 4: issue_book(); break;
            case 5: return_book(); break;
            case 6: reserve_book(); break;
            case 7: cancel_reservation(); break;
            case 8: show_fines(); break;
            case 9: sort_members_by_fine(); break;
            case 10: pay_fine(); break;
            case 11: show_fines_by_category(); break;
            case 0: break;
            default: printf("Invalid menu choice.\n");
        }
    } while (choice != 0);
}

/* ------------------------- MAIN PROGRAM ------------------------- */
int main(void) {
    int choice;
    printf("==============================================\n");
    printf("          LIBRARY MANAGEMENT SYSTEM\n");
    printf("==============================================\n");
    (void)load_books();
    (void)load_members();
    printf("Loaded %d book record(s) and %d member record(s).\n", book_count, member_count);

    do {
        printf("\n================ MAIN MENU ================\n");
        printf("1. Admin / Book Catalog\n");
        printf("2. Members / Issue / Return / Fines\n");
        printf("3. Display all books\n");
        printf("4. Save all records now\n");
        printf("0. Save and exit\n");
        if (!read_int("Enter choice: ", 0, 4, &choice)) choice = 0;
        switch (choice) {
            case 1: admin_menu(); break;
            case 2: member_menu(); break;
            case 3: display_books(); break;
            case 4: save_all(); break;
            case 0: save_all(); printf("Thank you for using the Library Management System.\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 0);

    free(books);
    free(members);
    books = NULL;
    members = NULL;
    return 0;
}
