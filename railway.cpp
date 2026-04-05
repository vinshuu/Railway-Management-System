/*
 * ============================================================
 *          RAILWAY MANAGEMENT SYSTEM
 *          A Console-Based C++ Application
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <limits>
#include <cctype>   // FIX: explicitly include for tolower / toupper

using namespace std;

const int  MAX_TRAINS   = 100;
const int  MAX_TICKETS  = 500;
const int  MAX_SEATS    = 200;
const char TRAIN_FILE[]  = "trains.dat";
const char TICKET_FILE[] = "tickets.dat";
const char ADMIN_PASS[]  = "admin123";

struct Train {
    int    trainNumber;
    char   trainName[50];
    char   source[40];
    char   destination[40];
    int    totalSeats;
    int    availableSeats;
    float  farePerSeat;
    char   departureTime[10];
    char   arrivalTime[10];
    bool   isActive;
};

struct Passenger {
    char name[50];
    int  age;
    char gender;
    char idProof[30];
};

struct Ticket {
    long long  pnrNumber;
    int        trainNumber;
    char       trainName[50];
    char       source[40];
    char       destination[40];
    Passenger  passenger;
    int        seatNumber;
    float      fare;
    char       bookingDate[15];
    bool       isCancelled;
};

Train  trains[MAX_TRAINS];
Ticket tickets[MAX_TICKETS];
int    trainCount  = 0;
int    ticketCount = 0;

// ─────────────────────────────────────────────
//  INPUT HELPERS
// ─────────────────────────────────────────────

void flushCin() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// Reads a line; assumes buffer is already clean (no leftover '\n').
void readStr(char* buf, int maxLen) {
    cin.getline(buf, maxLen);
}

void trimStr(char* str) {
    int len = (int)strlen(str);
    while (len > 0 &&
           (str[len-1] == '\r' || str[len-1] == ' ' || str[len-1] == '\n')) {
        str[--len] = '\0';
    }
}

bool strEqCI(const char* a, const char* b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return false;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

// ─────────────────────────────────────────────
//  UTILITY
// ─────────────────────────────────────────────

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pauseScreen() {
    cout << "\n  Press ENTER to continue...";
    flushCin();
}

void printLine(char ch = '=', int len = 60) {
    for (int i = 0; i < len; i++) cout << ch;
    cout << "\n";
}

void printHeader(const char* title) {
    clearScreen();
    printLine('=');
    int spaces = (60 - (int)strlen(title)) / 2;
    if (spaces < 0) spaces = 0;
    for (int i = 0; i < spaces; i++) cout << " ";
    cout << title << "\n";
    printLine('=');
    cout << "\n";
}

// FIX: generate a globally unique PNR using a static counter to avoid
//      collisions when multiple bookings happen within the same second.
long long generatePNR() {
    static int seq = 0;
    return (long long)time(nullptr) * 1000LL + (seq++ % 1000);
}

void getToday(char* buf) {
    time_t t = time(nullptr);
    tm* lt   = localtime(&t);
    sprintf(buf, "%02d/%02d/%04d",
            lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900);
}

// ─────────────────────────────────────────────
//  FILE HANDLING
// ─────────────────────────────────────────────

void saveTrains() {
    ofstream f(TRAIN_FILE, ios::binary | ios::trunc);
    if (!f) { cout << "  [Error] Cannot save train data!\n"; return; }
    f.write(reinterpret_cast<char*>(&trainCount), sizeof(trainCount));
    f.write(reinterpret_cast<char*>(trains), sizeof(Train) * trainCount);
    f.close();
}

void loadTrains() {
    ifstream f(TRAIN_FILE, ios::binary);
    if (!f) { trainCount = 0; return; }
    f.read(reinterpret_cast<char*>(&trainCount), sizeof(trainCount));
    if (trainCount < 0 || trainCount > MAX_TRAINS) trainCount = 0;
    f.read(reinterpret_cast<char*>(trains), sizeof(Train) * trainCount);
    f.close();
}

void saveTickets() {
    ofstream f(TICKET_FILE, ios::binary | ios::trunc);
    if (!f) { cout << "  [Error] Cannot save ticket data!\n"; return; }
    f.write(reinterpret_cast<char*>(&ticketCount), sizeof(ticketCount));
    f.write(reinterpret_cast<char*>(tickets), sizeof(Ticket) * ticketCount);
    f.close();
}

void loadTickets() {
    ifstream f(TICKET_FILE, ios::binary);
    if (!f) { ticketCount = 0; return; }
    f.read(reinterpret_cast<char*>(&ticketCount), sizeof(ticketCount));
    if (ticketCount < 0 || ticketCount > MAX_TICKETS) ticketCount = 0;
    f.read(reinterpret_cast<char*>(tickets), sizeof(Ticket) * ticketCount);
    f.close();
}

// ─────────────────────────────────────────────
//  DISPLAY HELPERS
// ─────────────────────────────────────────────

void printTrainRow(const Train& t) {
    cout << "  " << left
         << setw(8)  << t.trainNumber
         << setw(22) << t.trainName
         << setw(14) << t.source
         << setw(14) << t.destination
         << setw(7)  << t.departureTime
         << setw(7)  << t.arrivalTime
         << setw(8)  << t.availableSeats
         << "Rs." << fixed << setprecision(2) << t.farePerSeat
         << "\n";
}

void printTrainHeader() {
    printLine('-');
    cout << "  " << left
         << setw(8)  << "TrainNo"
         << setw(22) << "Train Name"
         << setw(14) << "Source"
         << setw(14) << "Destination"
         << setw(7)  << "Dep."
         << setw(7)  << "Arr."
         << setw(8)  << "Avail."
         << "Fare\n";
    printLine('-');
}

void printTicketDetails(const Ticket& tk) {
    printLine('-');
    cout << "  PNR Number    : " << tk.pnrNumber         << "\n";
    cout << "  Train No.     : " << tk.trainNumber        << "\n";
    cout << "  Train Name    : " << tk.trainName          << "\n";
    cout << "  From          : " << tk.source             << "\n";
    cout << "  To            : " << tk.destination        << "\n";
    cout << "  Passenger     : " << tk.passenger.name     << "\n";
    cout << "  Age / Gender  : " << tk.passenger.age
         << " / "               << tk.passenger.gender   << "\n";
    cout << "  ID Proof      : " << tk.passenger.idProof  << "\n";
    cout << "  Seat Number   : " << tk.seatNumber         << "\n";
    cout << "  Fare Paid     : Rs." << fixed << setprecision(2)
         << tk.fare                                       << "\n";
    cout << "  Booking Date  : " << tk.bookingDate        << "\n";
    cout << "  Status        : "
         << (tk.isCancelled ? "CANCELLED" : "CONFIRMED") << "\n";
    printLine('-');
}

// ─────────────────────────────────────────────
//  FIND HELPERS
// ─────────────────────────────────────────────

int findTrainIndex(int trainNo) {
    for (int i = 0; i < trainCount; i++)
        if (trains[i].trainNumber == trainNo && trains[i].isActive)
            return i;
    return -1;
}

// FIX: also find inactive trains (needed when restoring seats on cancel)
int findTrainIndexAny(int trainNo) {
    for (int i = 0; i < trainCount; i++)
        if (trains[i].trainNumber == trainNo)
            return i;
    return -1;
}

int findTicketIndex(long long pnr) {
    for (int i = 0; i < ticketCount; i++)
        if (tickets[i].pnrNumber == pnr)
            return i;
    return -1;
}

// FIX: seat search is now bounded by the train's own totalSeats,
//      not the global MAX_SEATS, preventing over-booking.
int nextSeat(int trainIdx) {
    bool used[MAX_SEATS + 1];
    memset(used, false, sizeof(used));
    int trainNo   = trains[trainIdx].trainNumber;
    int maxSeat   = trains[trainIdx].totalSeats;     // FIX

    for (int i = 0; i < ticketCount; i++) {
        Ticket& tk = tickets[i];
        if (tk.trainNumber == trainNo && !tk.isCancelled
            && tk.seatNumber >= 1 && tk.seatNumber <= maxSeat)
            used[tk.seatNumber] = true;
    }
    for (int s = 1; s <= maxSeat; s++)               // FIX
        if (!used[s]) return s;
    return -1;
}

// ─────────────────────────────────────────────
//  ADMIN FUNCTIONS
// ─────────────────────────────────────────────

bool adminLogin() {
    char pass[30];
    cout << "  Enter Admin Password: ";
    cin >> pass; flushCin();
    if (strcmp(pass, ADMIN_PASS) == 0) {
        cout << "\n  Login successful!\n";
        return true;
    }
    cout << "\n  [Error] Incorrect password!\n";
    return false;
}

void addTrain() {
    printHeader("ADD NEW TRAIN");
    if (trainCount >= MAX_TRAINS) {
        cout << "  [Error] Train database is full!\n";
        pauseScreen(); return;
    }
    Train t;

    cout << "  Enter Train Number   : ";
    cin >> t.trainNumber; flushCin();

    if (findTrainIndex(t.trainNumber) != -1) {
        cout << "  [Error] Train number already exists!\n";
        pauseScreen(); return;
    }

    cout << "  Enter Train Name     : "; readStr(t.trainName, 50);   trimStr(t.trainName);
    cout << "  Enter Source         : "; readStr(t.source, 40);      trimStr(t.source);
    cout << "  Enter Destination    : "; readStr(t.destination, 40); trimStr(t.destination);

    cout << "  Total Seats          : ";
    cin >> t.totalSeats; flushCin();
    if (t.totalSeats <= 0 || t.totalSeats > MAX_SEATS) {
        cout << "  [Error] Invalid seat count (1-" << MAX_SEATS << ")!\n";
        pauseScreen(); return;
    }
    t.availableSeats = t.totalSeats;

    cout << "  Fare Per Seat (Rs.)  : ";
    cin >> t.farePerSeat; flushCin();

    // FIX: use readStr (with size limit) for time strings instead of
    //      cin >> to avoid possible buffer overflow for long inputs.
    cout << "  Departure Time(HH:MM): "; readStr(t.departureTime, 10); trimStr(t.departureTime);
    cout << "  Arrival   Time(HH:MM): "; readStr(t.arrivalTime,   10); trimStr(t.arrivalTime);

    t.isActive = true;
    trains[trainCount++] = t;
    saveTrains();
    cout << "\n  Train added successfully!\n";
    pauseScreen();
}

void viewAllTrains() {
    printHeader("ALL TRAINS");
    bool found = false;
    printTrainHeader();
    for (int i = 0; i < trainCount; i++) {
        if (trains[i].isActive) {
            printTrainRow(trains[i]);
            found = true;
        }
    }
    if (!found) cout << "  No trains found.\n";
    printLine('-');
    pauseScreen();
}

void updateTrain() {
    printHeader("UPDATE TRAIN");
    cout << "  Enter Train Number to update: ";
    int no; cin >> no; flushCin();
    int idx = findTrainIndex(no);
    if (idx == -1) { cout << "  [Error] Train not found!\n"; pauseScreen(); return; }
    Train& t = trains[idx];
    cout << "\n  Current Details:\n";
    printTrainHeader();
    printTrainRow(t);
    cout << "\n  What to update?\n";
    cout << "  1. Train Name\n  2. Departure Time\n  3. Arrival Time\n"
         << "  4. Fare Per Seat\n  5. Total Seats\n  0. Cancel\n";
    int ch; cout << "  Choice: "; cin >> ch; flushCin();
    switch (ch) {
        case 1:
            cout << "  New Train Name     : ";
            readStr(t.trainName, 50); trimStr(t.trainName);
            break;
        case 2:
            // FIX: use readStr for time fields (same buffer-overflow fix as addTrain)
            cout << "  New Departure Time : ";
            readStr(t.departureTime, 10); trimStr(t.departureTime);
            break;
        case 3:
            cout << "  New Arrival Time   : ";
            readStr(t.arrivalTime, 10); trimStr(t.arrivalTime);
            break;
        case 4:
            cout << "  New Fare (Rs.)     : ";
            cin >> t.farePerSeat; flushCin();
            break;
        case 5: {
            int ns; cout << "  New Total Seats    : "; cin >> ns; flushCin();
            int booked = t.totalSeats - t.availableSeats;
            if (ns < booked) {
                cout << "  [Error] Cannot reduce below booked seats ("
                     << booked << ")!\n";
                break;
            }
            t.availableSeats = ns - booked;
            t.totalSeats = ns;
            break;
        }
        case 0: return;
        default: cout << "  Invalid choice.\n";
    }
    saveTrains();
    cout << "\n  Train updated successfully!\n";
    pauseScreen();
}

void deleteTrain() {
    printHeader("DELETE TRAIN");
    cout << "  Enter Train Number to delete: ";
    int no; cin >> no; flushCin();
    int idx = findTrainIndex(no);
    if (idx == -1) { cout << "  [Error] Train not found!\n"; pauseScreen(); return; }
    for (int i = 0; i < ticketCount; i++) {
        if (tickets[i].trainNumber == no && !tickets[i].isCancelled) {
            cout << "  [Error] Train has active bookings. Cancel them first!\n";
            pauseScreen(); return;
        }
    }
    char confirm;
    cout << "  Confirm delete? (y/n): "; cin >> confirm; flushCin();
    if (tolower((unsigned char)confirm) == 'y') {
        trains[idx].isActive = false;
        saveTrains();
        cout << "  Train deleted successfully!\n";
    } else {
        cout << "  Deletion cancelled.\n";
    }
    pauseScreen();
}

void viewBookingsByTrain() {
    printHeader("BOOKINGS BY TRAIN");
    cout << "  Enter Train Number: ";
    int no; cin >> no; flushCin();
    int idx = findTrainIndex(no);
    if (idx == -1) { cout << "  [Error] Train not found!\n"; pauseScreen(); return; }
    cout << "\n  Train: " << trains[idx].trainName << " (" << no << ")\n\n";
    printLine('-');
    cout << "  " << left
         << setw(14) << "PNR"
         << setw(22) << "Passenger"
         << setw(5)  << "Age"
         << setw(5)  << "Sex"
         << setw(8)  << "Seat"
         << setw(12) << "Fare"
         << "Status\n";
    printLine('-');
    bool found = false;
    for (int i = 0; i < ticketCount; i++) {
        Ticket& tk = tickets[i];
        if (tk.trainNumber == no) {
            cout << "  " << left
                 << setw(14) << tk.pnrNumber
                 << setw(22) << tk.passenger.name
                 << setw(5)  << tk.passenger.age
                 << setw(5)  << tk.passenger.gender
                 << setw(8)  << tk.seatNumber
                 << "Rs." << setw(9) << fixed << setprecision(2) << tk.fare
                 << (tk.isCancelled ? "CANCELLED" : "CONFIRMED") << "\n";
            found = true;
        }
    }
    if (!found) cout << "  No bookings found for this train.\n";
    printLine('-');
    pauseScreen();
}

void adminMenu() {
    printHeader("ADMIN LOGIN");
    if (!adminLogin()) { pauseScreen(); return; }
    int choice;
    do {
        printHeader("ADMIN PANEL");
        cout << "  1. Add Train\n"
             << "  2. View All Trains\n"
             << "  3. Update Train Details\n"
             << "  4. Delete Train\n"
             << "  5. View Bookings by Train\n"
             << "  0. Logout\n\n"
             << "  Enter Choice: ";
        cin >> choice; flushCin();
        switch (choice) {
            case 1: addTrain();            break;
            case 2: viewAllTrains();       break;
            case 3: updateTrain();         break;
            case 4: deleteTrain();         break;
            case 5: viewBookingsByTrain(); break;
            case 0: cout << "\n  Logged out.\n"; break;
            default: cout << "  Invalid choice!\n"; pauseScreen();
        }
    } while (choice != 0);
}

// ─────────────────────────────────────────────
//  PASSENGER FUNCTIONS
// ─────────────────────────────────────────────

void searchTrains() {
    printHeader("SEARCH TRAINS");
    char src[40], dest[40];
    cout << "  Enter Source      : "; readStr(src, 40);  trimStr(src);
    cout << "  Enter Destination : "; readStr(dest, 40); trimStr(dest);
    cout << "\n  Search Results:\n";
    printTrainHeader();
    bool found = false;
    for (int i = 0; i < trainCount; i++) {
        Train& t = trains[i];
        if (t.isActive &&
            strEqCI(t.source, src) &&
            strEqCI(t.destination, dest)) {
            printTrainRow(t);
            found = true;
        }
    }
    if (!found) cout << "  No trains found for this route.\n";
    printLine('-');
    pauseScreen();
}

void checkSeatAvailability() {
    printHeader("CHECK SEAT AVAILABILITY");
    cout << "  Enter Train Number: ";
    int no; cin >> no; flushCin();
    int idx = findTrainIndex(no);
    if (idx == -1) { cout << "  [Error] Train not found!\n"; pauseScreen(); return; }
    Train& t = trains[idx];
    cout << "\n  Train     : " << t.trainName    << "\n"
         << "  Route     : " << t.source << " --> " << t.destination << "\n"
         << "  Departure : " << t.departureTime  << "\n"
         << "  Arrival   : " << t.arrivalTime    << "\n"
         << "  Fare/Seat : Rs." << fixed << setprecision(2) << t.farePerSeat << "\n";
    printLine('-');
    cout << "  Total Seats     : " << t.totalSeats                      << "\n"
         << "  Booked Seats    : " << (t.totalSeats - t.availableSeats) << "\n"
         << "  Available Seats : " << t.availableSeats                  << "\n";
    printLine('-');
    if (t.availableSeats > 0)
        cout << "  STATUS: SEATS AVAILABLE\n";
    else
        cout << "  STATUS: FULLY BOOKED\n";
    pauseScreen();
}

void bookTicket() {
    printHeader("BOOK TICKET");
    if (ticketCount >= MAX_TICKETS) {
        cout << "  [Error] Ticket database full!\n"; pauseScreen(); return;
    }
    cout << "  Enter Train Number: ";
    int no; cin >> no; flushCin();
    int idx = findTrainIndex(no);
    if (idx == -1) { cout << "  [Error] Train not found!\n"; pauseScreen(); return; }
    Train& t = trains[idx];
    if (t.availableSeats <= 0) {
        cout << "  [Error] No seats available!\n"; pauseScreen(); return;
    }
    cout << "\n  Train : " << t.trainName << "\n"
         << "  Route : " << t.source << " --> " << t.destination << "\n"
         << "  Fare  : Rs." << fixed << setprecision(2) << t.farePerSeat << "\n\n";

    Ticket tk;
    tk.trainNumber = no;
    strcpy(tk.trainName,   t.trainName);
    strcpy(tk.source,      t.source);
    strcpy(tk.destination, t.destination);
    tk.fare        = t.farePerSeat;
    tk.isCancelled = false;

    cout << "  --- Passenger Details ---\n";
    cout << "  Name          : "; readStr(tk.passenger.name, 50); trimStr(tk.passenger.name);

    cout << "  Age           : "; cin >> tk.passenger.age; flushCin();
    if (tk.passenger.age <= 0 || tk.passenger.age > 120) {
        cout << "  [Error] Invalid age!\n"; pauseScreen(); return;
    }

    cout << "  Gender (M/F/O): "; cin >> tk.passenger.gender; flushCin();
    tk.passenger.gender = (char)toupper((unsigned char)tk.passenger.gender);

    cout << "  ID Proof      : "; readStr(tk.passenger.idProof, 30); trimStr(tk.passenger.idProof);

    // FIX: pass train index (not train number) to nextSeat so it respects totalSeats
    tk.seatNumber = nextSeat(idx);
    if (tk.seatNumber == -1) {
        cout << "  [Error] No seat could be assigned!\n"; pauseScreen(); return;
    }
    tk.pnrNumber = generatePNR();
    getToday(tk.bookingDate);

    cout << "\n  --- Booking Summary ---\n";
    printLine('-');
    cout << "  Train      : " << tk.trainName          << "\n"
         << "  Route      : " << tk.source << " --> " << tk.destination << "\n"
         << "  Passenger  : " << tk.passenger.name      << "\n"
         << "  Age/Gender : " << tk.passenger.age << " / " << tk.passenger.gender << "\n"
         << "  Seat No.   : " << tk.seatNumber          << "\n"
         << "  Fare       : Rs." << fixed << setprecision(2) << tk.fare << "\n";
    printLine('-');
    char confirm;
    cout << "\n  Confirm Booking? (y/n): "; cin >> confirm; flushCin();
    if (tolower((unsigned char)confirm) != 'y') {
        cout << "  Booking cancelled.\n"; pauseScreen(); return;
    }

    tickets[ticketCount++] = tk;
    t.availableSeats--;
    saveTickets();
    saveTrains();

    cout << "\n  *** BOOKING CONFIRMED ***\n";
    printLine('*');
    cout << "  PNR Number : " << tk.pnrNumber << "\n";
    printLine('*');
    cout << "  Please note your PNR for cancellation/status checks.\n";
    pauseScreen();
}

void cancelTicket() {
    printHeader("CANCEL TICKET");
    cout << "  Enter PNR Number: ";
    long long pnr; cin >> pnr; flushCin();
    int idx = findTicketIndex(pnr);
    if (idx == -1) { cout << "  [Error] PNR not found!\n"; pauseScreen(); return; }
    Ticket& tk = tickets[idx];
    if (tk.isCancelled) {
        cout << "  [Error] Ticket already cancelled!\n"; pauseScreen(); return;
    }
    cout << "\n  Ticket Details:\n";
    printTicketDetails(tk);
    char confirm;
    cout << "  Confirm Cancellation? (y/n): "; cin >> confirm; flushCin();
    if (tolower((unsigned char)confirm) != 'y') {
        cout << "  Cancellation aborted.\n"; pauseScreen(); return;
    }

    tk.isCancelled = true;
    // FIX: use findTrainIndexAny so cancelled/deleted trains still
    //      have their available seat count restored correctly.
    int tidx = findTrainIndexAny(tk.trainNumber);
    if (tidx != -1) {
        trains[tidx].availableSeats++;
        saveTrains();
    }
    saveTickets();
    float refund = tk.fare * 0.80f;
    cout << "\n  Ticket cancelled successfully!\n";
    cout << "  Refund Amount (80%) : Rs." << fixed << setprecision(2)
         << refund << "\n";
    pauseScreen();
}

void viewTicketByPNR() {
    printHeader("VIEW TICKET");
    cout << "  Enter PNR Number: ";
    long long pnr; cin >> pnr; flushCin();
    int idx = findTicketIndex(pnr);
    if (idx == -1) { cout << "  [Error] PNR not found!\n"; pauseScreen(); return; }
    cout << "\n  Ticket Details:\n";
    printTicketDetails(tickets[idx]);
    pauseScreen();
}

void viewMyTickets() {
    printHeader("MY TICKETS");
    char name[50];
    cout << "  Enter Passenger Name: "; readStr(name, 50); trimStr(name);
    bool found = false;
    for (int i = 0; i < ticketCount; i++) {
        if (strEqCI(tickets[i].passenger.name, name)) {
            printTicketDetails(tickets[i]);
            found = true;
        }
    }
    if (!found) cout << "  No tickets found for this name.\n";
    pauseScreen();
}

void fareCalculator() {
    printHeader("FARE CALCULATOR");
    cout << "  Enter Train Number: ";
    int no; cin >> no; flushCin();
    int idx = findTrainIndex(no);
    if (idx == -1) { cout << "  [Error] Train not found!\n"; pauseScreen(); return; }
    cout << "  Number of Seats   : ";
    int seats; cin >> seats; flushCin();
    if (seats <= 0) { cout << "  [Error] Invalid seat count!\n"; pauseScreen(); return; }
    float total = trains[idx].farePerSeat * (float)seats;
    cout << "\n  Train       : " << trains[idx].trainName << "\n"
         << "  Fare/Seat   : Rs." << fixed << setprecision(2) << trains[idx].farePerSeat << "\n"
         << "  Seats       : " << seats << "\n";
    printLine('-');
    cout << "  Total Fare  : Rs." << fixed << setprecision(2) << total << "\n";
    printLine('-');
    pauseScreen();
}

void passengerMenu() {
    int choice;
    do {
        printHeader("PASSENGER PORTAL");
        cout << "  1. Search Trains\n"
             << "  2. Check Seat Availability\n"
             << "  3. Book Ticket\n"
             << "  4. Cancel Ticket\n"
             << "  5. View Ticket (by PNR)\n"
             << "  6. View My Tickets (by Name)\n"
             << "  7. Fare Calculator\n"
             << "  0. Back to Main Menu\n\n"
             << "  Enter Choice: ";
        cin >> choice; flushCin();
        switch (choice) {
            case 1: searchTrains();          break;
            case 2: checkSeatAvailability(); break;
            case 3: bookTicket();            break;
            case 4: cancelTicket();          break;
            case 5: viewTicketByPNR();       break;
            case 6: viewMyTickets();         break;
            case 7: fareCalculator();        break;
            case 0: break;
            default: cout << "  Invalid choice!\n"; pauseScreen();
        }
    } while (choice != 0);
}

// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────

void splashScreen() {
    clearScreen();
    printLine('*');
    cout << "\n";
    cout << "        RAILWAY MANAGEMENT SYSTEM\n";
    cout << "        Console Application  |  C++\n";
    cout << "\n";
    printLine('*');
    cout << "\n  Loading saved data...\n";
    loadTrains();
    loadTickets();
    cout << "  Trains loaded  : " << trainCount  << "\n";
    cout << "  Tickets loaded : " << ticketCount << "\n";
    cout << "\n  Press ENTER to continue...";
    cin.get();   // stream is clean at program start
}

int main() {
    srand((unsigned)time(nullptr));
    splashScreen();
    int choice;
    do {
        printHeader("MAIN MENU");
        cout << "  1. Admin Panel\n"
             << "  2. Passenger Portal\n"
             << "  3. View All Trains\n"
             << "  0. Exit\n\n"
             << "  Enter Choice: ";
        cin >> choice; flushCin();
        switch (choice) {
            case 1: adminMenu();     break;
            case 2: passengerMenu(); break;
            case 3: viewAllTrains(); break;
            case 0:
                clearScreen();
                printLine('=');
                cout << "  Thank you for using Railway Management System!\n";
                printLine('=');
                break;
            default:
                cout << "  Invalid choice!\n";
                pauseScreen();
        }
    } while (choice != 0);
    return 0;
}
