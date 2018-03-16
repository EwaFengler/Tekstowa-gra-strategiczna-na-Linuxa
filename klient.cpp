#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include <ncurses.h>
#include <unistd.h>

//SYGNAL NA ROZPOCZECIE GRY
#define WEJSCIE 15

//DO KLIENTA
#define ROZLACZENIE 14
#define WYGRANA 13
#define PRZEGRANA 12
#define SUKCES_WROGA 11
#define PORAZKA_WROGA 10
#define UDANY_ATAK 9
#define NIEUDANY_ATAK 8
#define NOWE_DANE 7
#define ZLA_LICZBA 6
#define STAN_GRY 5

//OD KLIENTA
#define PRODUKUJ 4
#define ATAKUJ 3
#define WYJSCIE 2

using namespace std;

typedef struct wiad_deskryptor_struktura{
  long typ;
  int desk;
} wiad_deskryptor;

typedef struct sygnal_struktura{
  long typ;
} sygnal;

typedef struct wiad_atak_struktura{
  long typ;
  short ile_lekkiej_piechoty;
  short ile_ciezkiej_piechoty;
  short ile_jazdy;
  short nr_wroga;
} wiad_atak;

typedef struct wiad_produkcja_struktura{
  long typ;
  short nr_jednostki;
  short ile;
} wiad_produkcja;

typedef struct stan_gry_struktura{
  long typ;
  int surowce1;
  short ile_lekkiej_piechoty1;
  short ile_ciezkiej_piechoty1;
  short ile_jazdy1;
  short ile_robotnikow1;
  short ile_wygranych1;
  int surowce2;
  short ile_lekkiej_piechoty2;
  short ile_ciezkiej_piechoty2;
  short ile_jazdy2;
  short ile_robotnikow2;
  short ile_wygranych2;
  int surowce3;
  short ile_lekkiej_piechoty3;
  short ile_ciezkiej_piechoty3;
  short ile_jazdy3;
  short ile_robotnikow3;
  short ile_wygranych3;
} stan_gry;

void wypisz_stan(stan_gry stan_gry1){
  mvprintw(1, 0, "nr gracza:");
  mvprintw(2, 0, "surowce:");
  mvprintw(3, 0, "lekka piechota:");
  mvprintw(4, 0, "ciezka piechota:");
  mvprintw(5, 0, "jazda:");
  mvprintw(6, 0, "robotnicy:");
  mvprintw(7, 0, "wygranych:");

  mvprintw(1, 25, "%d\t\t%d\t\t%d", 0, 1, 2);
  mvprintw(2, 25, "%d\t\t%d\t\t%d", stan_gry1.surowce1, stan_gry1.surowce2, stan_gry1.surowce3);
  mvprintw(3, 25, "%d\t\t%d\t\t%d", stan_gry1.ile_lekkiej_piechoty1, stan_gry1.ile_lekkiej_piechoty2, stan_gry1.ile_lekkiej_piechoty3);
  mvprintw(4, 25, "%d\t\t%d\t\t%d", stan_gry1.ile_ciezkiej_piechoty1, stan_gry1.ile_ciezkiej_piechoty2, stan_gry1.ile_ciezkiej_piechoty3);
  mvprintw(5, 25, "%d\t\t%d\t\t%d", stan_gry1.ile_jazdy1, stan_gry1.ile_jazdy2, stan_gry1.ile_jazdy3);
  mvprintw(6, 25, "%d\t\t%d\t\t%d", stan_gry1.ile_robotnikow1, stan_gry1.ile_robotnikow2, stan_gry1.ile_robotnikow3);
  mvprintw(7, 25, "%d\t\t%d\t\t%d", stan_gry1.ile_wygranych1, stan_gry1.ile_wygranych2, stan_gry1.ile_wygranych3);
}

int kol_klient;

void powiadom_o_wyjsciu(int nr){
  sygnal sygnal1 = {WYJSCIE};
  msgsnd(kol_klient, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
  exit(0);
}

int main(){
  signal(SIGHUP, powiadom_o_wyjsciu);
  signal(SIGINT, powiadom_o_wyjsciu);
  signal(SIGQUIT, powiadom_o_wyjsciu);
  signal(SIGTERM, powiadom_o_wyjsciu);

  int kol_polaczenie = msgget(11223344, 0);
  kol_klient = msgget(IPC_PRIVATE, 0640 | IPC_CREAT | IPC_EXCL);

  sygnal sygnal1;
  stan_gry stan_gry1;

  wiad_deskryptor wiad_deskryptor1 = {WEJSCIE, kol_klient};

  //wyslanie prosby o dolaczenie do gry
  msgsnd(kol_polaczenie, &wiad_deskryptor1, sizeof(wiad_deskryptor) - sizeof(long), 0);
  printf("Wyslano prosbe\n");

  msgrcv(kol_klient, &stan_gry1, sizeof(stan_gry) - sizeof(long), STAN_GRY, 0);

  //rozpoczecie gry
  initscr();
  cbreak();
  nodelay(stdscr, true);
  char c, tab[10];

  wypisz_stan(stan_gry1);

  while(true){
    msgrcv(kol_klient, &sygnal1, sizeof(sygnal1) - sizeof(long), 0, IPC_NOWAIT);

    //obsluga sygnalow od serwera
    if(sygnal1.typ != -1){
      if(sygnal1.typ == ROZLACZENIE){
       clear();
       mvprintw(LINES / 2, (COLS-53) / 2, "Program serwera lub innego gracza przestal dzialac.");
       refresh();
       break;

      } else if(sygnal1.typ == ZLA_LICZBA){
        mvprintw(0, 0, "Komunikat: podales zla liczbe! Sprobuj jeszcze raz.\n");

      } else if(sygnal1.typ == NOWE_DANE){
        msgrcv(kol_klient, &stan_gry1, sizeof(stan_gry) - sizeof(long), STAN_GRY, 0);
        wypisz_stan(stan_gry1);

      } else if(sygnal1.typ == UDANY_ATAK){
        mvprintw(0, 0, "Komunikat: wojska powrocily po zwycieskim ataku!\n");
        msgrcv(kol_klient, &stan_gry1, sizeof(stan_gry) - sizeof(long), STAN_GRY, 0);
        wypisz_stan(stan_gry1);

      } else if(sygnal1.typ == NIEUDANY_ATAK){
        mvprintw(0, 0, "Komunikat: szczatki wojsk powrocily po druzgoczacej porazce!\n");
        msgrcv(kol_klient, &stan_gry1, sizeof(stan_gry) - sizeof(long), STAN_GRY, 0);
        wypisz_stan(stan_gry1);

      } else if(sygnal1.typ == SUKCES_WROGA){
        mvprintw(0, 0, "Komunikat: zostales zaatakowany! Twoje wojsko zostalo rozgromione.\n");
        msgrcv(kol_klient, &stan_gry1, sizeof(stan_gry) - sizeof(long), STAN_GRY, 0);
        wypisz_stan(stan_gry1);

      } else if(sygnal1.typ == PORAZKA_WROGA){
        mvprintw(0, 0, "Komunikat: zostales zaatakowany! Szczesliwie, odparles atak.\n");
        msgrcv(kol_klient, &stan_gry1, sizeof(stan_gry) - sizeof(long), STAN_GRY, 0);
        wypisz_stan(stan_gry1);

      } else if(sygnal1.typ == WYGRANA){
        clear();
        mvprintw(LINES / 2, (COLS-27) / 2, "GRATULACJE, WYGRALES! :>");
        break;

      } else if(sygnal1.typ == PRZEGRANA){
        clear();
        mvprintw(LINES / 2, (COLS-27) / 2, "NIESTETY, PRZEGRALES! :<");
        break;
      }
    }

    mvprintw(9, 0, "nacisnij dowolny klawisz, aby wybrac akcje\n");

    //obsluga polecen gracza
    if(getch() != ERR){
      nodelay(stdscr, false);

      clear();
      printw("0) produkcja\n1) atak\n> ");
      refresh();

      switch(getch()){
        case '0':
        printw("\n0) lekka piechota\n1) ciezka piechota\n2) jazda\n3) robotnicy\n> ");
        c = getch();

        if(c - '0' >= 0 && c - '0' <= 3){
          printw("\nile?:\n> ");
          getstr(tab);

          wiad_produkcja wiad_produkcja1 = {PRODUKUJ, (short)(c - '0'), (short)atoi(tab)};
          msgsnd(kol_klient, &wiad_produkcja1, sizeof(wiad_produkcja) - sizeof(long), 0);
        }
        break;

        case '1':
        printw("\ngracz 1 czy 2?\n> ");
        c = getch();

        if(c == '1' || c == '2'){
          short ile1, ile2, ile3;
          printw("\nIle wyslac lekkiej piechoty?\n> ");
          getstr(tab);
          ile1 = atoi(tab);

          printw("Ile wyslac ciezkiej piechoty?\n> ");
          getstr(tab);
          ile2 = atoi(tab);

          printw("Ile wyslac jazdy?\n> ");
          getstr(tab);
          ile3 = atoi(tab);

          wiad_atak wiad_atak1 = {ATAKUJ, ile1, ile2, ile3, (short)(c - '0')};
          msgsnd(kol_klient, &wiad_atak1, sizeof(wiad_atak) - sizeof(long), 0);
        }

        break;

        default:
        break;
      }
      //koniec obslugi polecen gracza
      nodelay(stdscr, true);
      clear();
      wypisz_stan(stan_gry1);
    }
  }

  nodelay(stdscr, false);
  getch();
  endwin();

  msgctl(kol_klient, IPC_RMID,0);
  return 0;
}
