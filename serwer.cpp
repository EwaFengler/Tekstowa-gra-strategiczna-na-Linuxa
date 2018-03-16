#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>

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

typedef struct jednostka_struktura{
  short cena;
  double atak;
  double obrona;
  short czasProdukcji;
} jednostka;

typedef struct status_gracza_struktura{
  int surowce;
  short ile_lekkiej_piechoty;
  short ile_ciezkiej_piechoty;
  short ile_jazdy;
  short ile_robotnikow;
  short ile_wygranych;
  int desk;
} status_gracza;

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

stan_gry generuj_stan(int nr_gracza, status_gracza statusy[3]){
  stan_gry stan_gry1 = {
    STAN_GRY,

    statusy[nr_gracza].surowce,
    statusy[nr_gracza].ile_lekkiej_piechoty,
    statusy[nr_gracza].ile_ciezkiej_piechoty,
    statusy[nr_gracza].ile_jazdy,
    statusy[nr_gracza].ile_robotnikow,
    statusy[nr_gracza].ile_wygranych,

    statusy[(nr_gracza+1)%3].surowce,
    statusy[(nr_gracza+1)%3].ile_lekkiej_piechoty,
    statusy[(nr_gracza+1)%3].ile_ciezkiej_piechoty,
    statusy[(nr_gracza+1)%3].ile_jazdy,
    statusy[(nr_gracza+1)%3].ile_robotnikow,
    statusy[(nr_gracza+1)%3].ile_wygranych,

    statusy[(nr_gracza+2)%3].surowce,
    statusy[(nr_gracza+2)%3].ile_lekkiej_piechoty,
    statusy[(nr_gracza+2)%3].ile_ciezkiej_piechoty,
    statusy[(nr_gracza+2)%3].ile_jazdy,
    statusy[(nr_gracza+2)%3].ile_robotnikow,
    statusy[(nr_gracza+2)%3].ile_wygranych
  };

  return stan_gry1;
}

status_gracza *statusy;

struct sembuf P = {0, -1, 0}, V = {0, 1, 0};
int sem;

void powiadom_o_rozlaczeniu(int nr){
  sygnal sygnal1 = {ROZLACZENIE};

  semop(sem, &P, 1);
  for(int i = 0; i < 3; i++){
    if(statusy[i].desk != -1){
      msgsnd(statusy[i].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
    }
  }
  semop(sem, &V, 1);

  exit(0);
}

int main(){
  signal(SIGHUP, powiadom_o_rozlaczeniu);
  signal(SIGINT, powiadom_o_rozlaczeniu);
  signal(SIGTERM, powiadom_o_rozlaczeniu);
  signal(SIGQUIT, powiadom_o_rozlaczeniu);

  int kol_polaczenie = msgget(11223344, 0640 | IPC_CREAT);


  jednostka lekka_piechota = {100, 1, 1.2, 2};
  jednostka ciezka_piechota = {250, 1.5, 3, 3};
  jednostka jazda = {550, 3.5, 1.2, 5};
  jednostka robotnik = {150, 0, 0, 2};

  jednostka jednostki[4] = {lekka_piechota, ciezka_piechota, jazda, robotnik};

  int ile_graczy_id = shmget (IPC_PRIVATE, sizeof(short), 0640 | IPC_CREAT);
  short *ile_graczy = (short*)shmat(ile_graczy_id, NULL, 0);
  *ile_graczy = 0;

  int statusy_id = shmget (IPC_PRIVATE, sizeof(status_gracza)*3, 0640 | IPC_CREAT);
  statusy = (status_gracza*)shmat(statusy_id, NULL, 0);

  status_gracza status_pocz = {0, 0, 0, 0, 0, 0, -1};
  statusy[0] = status_pocz;
  statusy[1] = status_pocz;
  statusy[2] = status_pocz;

  int trwa_gra_id = shmget (IPC_PRIVATE, sizeof(bool), 0640 | IPC_CREAT);
  bool *trwa_gra = (bool*)shmat(trwa_gra_id, NULL, 0);
  *trwa_gra = false;

  sem = semget(IPC_PRIVATE, 0, 0640 | IPC_CREAT);
  semctl(sem, 0, SETVAL, 1);

  /***** FORKI *****/
  int klient1 = fork();
  int klient2 = (klient1) ? fork() : 0;
  int klient3 = (klient1 && klient2) ? fork() : 0;

  //GRACZE
  if(!(klient1 && klient2 && klient3)){

    wiad_deskryptor wiad_deskryptor1;

    msgrcv(kol_polaczenie, &wiad_deskryptor1, sizeof(wiad_deskryptor) - sizeof(long), WEJSCIE, 0);
    status_gracza status = {300, 0, 0, 0, 0, 0, wiad_deskryptor1.desk};

    semop(sem, &P, 1);
    short nr_gracza = (*ile_graczy)++;
    statusy[nr_gracza] = status;
    semop(sem, &V, 1);

    printf("Wchodzi gracz %d\n", nr_gracza);

    //Wszyscy atak, mozna zaczac gre
    if(*ile_graczy == 3){
      semop(sem, &P, 1);
      *trwa_gra = true;
      semop(sem, &V, 1);
    }

    //czekanie na rozpoczecie
    while(true){
      semop(sem, &P, 1);
      if(*trwa_gra) {
          semop(sem, &V, 1);
          break;
      }
      semop(sem, &V, 1);
    }

    //powiadom_o_rozlaczeniuienie graczy o rozpoczeciu gry
    semop(sem, &P, 1);
    stan_gry stan_gry1 = generuj_stan(nr_gracza, statusy);
    semop(sem, &V, 1);
    msgsnd(statusy[nr_gracza].desk, &stan_gry1, sizeof(stan_gry1) - sizeof(long), 0);

    int generowanie_surowcow = fork();
    int realizacja_atakow = (generowanie_surowcow) ? fork() : -1;
    int realizacja_produkcji = (generowanie_surowcow && realizacja_atakow) ? fork() : -1;
    int rozlaczenie = (generowanie_surowcow && realizacja_atakow && realizacja_produkcji) ? fork() : -1;


    while(*trwa_gra){

      //SUROWCE
      if(!generowanie_surowcow){
        int tempo_produkcji;

        sleep(5);

        semop(sem, &P, 1);
        tempo_produkcji =  50 + 5 * statusy[nr_gracza].ile_robotnikow;
        statusy[nr_gracza].surowce += tempo_produkcji;

        stan_gry stan_gry1 = generuj_stan(nr_gracza, statusy);
        stan_gry stan_gry2 = generuj_stan((nr_gracza+1)%3, statusy);
        stan_gry stan_gry3 = generuj_stan((nr_gracza+2)%3, statusy);
        semop(sem, &V, 1);

        sygnal sygnal1 = {NOWE_DANE};
        msgsnd(statusy[nr_gracza].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
        msgsnd(statusy[nr_gracza].desk, &stan_gry1, sizeof(stan_gry1) - sizeof(long), 0);
        msgsnd(statusy[(nr_gracza+1)%3].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
        msgsnd(statusy[(nr_gracza+1)%3].desk, &stan_gry2, sizeof(stan_gry1) - sizeof(long), 0);
        msgsnd(statusy[(nr_gracza+2)%3].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
        msgsnd(statusy[(nr_gracza+2)%3].desk, &stan_gry3, sizeof(stan_gry1) - sizeof(long), 0);
      }

      //PRODUKCJA
      if(!realizacja_produkcji){
        wiad_produkcja wiad_produkcja1;
        msgrcv(statusy[nr_gracza].desk, &wiad_produkcja1, sizeof(wiad_produkcja) - sizeof(long), PRODUKUJ, 0);

        semop(sem, &P, 1);
        //sprawdzenie, czy polecenie produkcji jest poprawne
        if(statusy[nr_gracza].surowce >= jednostki[wiad_produkcja1.nr_jednostki].cena * wiad_produkcja1.ile){

          //odjecie kosztu produkcji
          statusy[nr_gracza].surowce -= jednostki[wiad_produkcja1.nr_jednostki].cena * wiad_produkcja1.ile;
          semop(sem, &V, 1);

          if(!fork()){//proces sukcesywnie produkujacy jednostki
            while(wiad_produkcja1.ile){
              sleep(jednostki[wiad_produkcja1.nr_jednostki].czasProdukcji);

              switch(wiad_produkcja1.nr_jednostki){
                case 0:
                  semop(sem, &P, 1);
                  statusy[nr_gracza].ile_lekkiej_piechoty++;
                  semop(sem, &V, 1);
                  break;

                case 1:
                  semop(sem, &P, 1);
                  statusy[nr_gracza].ile_ciezkiej_piechoty++;
                  semop(sem, &V, 1);
                  break;

                case 2:
                  semop(sem, &P, 1);
                  statusy[nr_gracza].ile_jazdy++;
                  semop(sem, &V, 1);
                  break;

                case 3:
                  semop(sem, &P, 1);
                  statusy[nr_gracza].ile_robotnikow++;
                  semop(sem, &V, 1);
                  break;

                default:
                  break;
              }
              semop(sem, &P, 1);
              stan_gry stan_gry1 = generuj_stan(nr_gracza, statusy);
              stan_gry stan_gry2 = generuj_stan((nr_gracza+1)%3, statusy);
              stan_gry stan_gry3 = generuj_stan((nr_gracza+2)%3, statusy);
              semop(sem, &V, 1);

              sygnal sygnal1 = {NOWE_DANE};
              msgsnd(statusy[nr_gracza].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
              msgsnd(statusy[nr_gracza].desk, &stan_gry1, sizeof(stan_gry1) - sizeof(long), 0);
              msgsnd(statusy[(nr_gracza+1)%3].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
              msgsnd(statusy[(nr_gracza+1)%3].desk, &stan_gry2, sizeof(stan_gry1) - sizeof(long), 0);
              msgsnd(statusy[(nr_gracza+2)%3].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
              msgsnd(statusy[(nr_gracza+2)%3].desk, &stan_gry3, sizeof(stan_gry1) - sizeof(long), 0);

              wiad_produkcja1.ile--;
            }

            exit(0);
          }

        } else {//niepoprawne dane produkcji
          semop(sem, &V, 1);
          sygnal sygnal1 = {ZLA_LICZBA};
          msgsnd(statusy[nr_gracza].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
        }
      }

      //ATAKI
      if(!realizacja_atakow){
        wiad_atak wiad_atak1;
        msgrcv(statusy[nr_gracza].desk, &wiad_atak1, sizeof(wiad_atak) - sizeof(long), ATAKUJ, 0);

        //sprawdzanie poprawnosci polecenia ataku
        semop(sem, &P, 1);
        bool mozna_atakowac = (wiad_atak1.ile_lekkiej_piechoty <= statusy[nr_gracza].ile_lekkiej_piechoty
            && wiad_atak1.ile_ciezkiej_piechoty <= statusy[nr_gracza].ile_ciezkiej_piechoty
            && wiad_atak1.ile_jazdy <= statusy[nr_gracza].ile_jazdy);

        if(mozna_atakowac){//polecenie jest poprawne
          short nr_wroga = (nr_gracza + wiad_atak1.nr_wroga) % 3;
          short inny_gracz = 3 - (nr_gracza + nr_wroga);

          statusy[nr_gracza].ile_lekkiej_piechoty -= wiad_atak1.ile_lekkiej_piechoty;
          statusy[nr_gracza].ile_ciezkiej_piechoty -= wiad_atak1.ile_ciezkiej_piechoty;
          statusy[nr_gracza].ile_jazdy -= wiad_atak1.ile_jazdy;

          stan_gry stan_gry1 = generuj_stan(nr_gracza, statusy);
          stan_gry stan_gry2 = generuj_stan((nr_gracza+1)%3, statusy);
          stan_gry stan_gry3 = generuj_stan((nr_gracza+2)%3, statusy);
          semop(sem, &V, 1);

          sygnal sygnal1 = {NOWE_DANE};
          msgsnd(statusy[nr_gracza].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
          msgsnd(statusy[nr_gracza].desk, &stan_gry1, sizeof(stan_gry1) - sizeof(long), 0);
          msgsnd(statusy[(nr_gracza+1)%3].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
          msgsnd(statusy[(nr_gracza+1)%3].desk, &stan_gry2, sizeof(stan_gry1) - sizeof(long), 0);
          msgsnd(statusy[(nr_gracza+2)%3].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
          msgsnd(statusy[(nr_gracza+2)%3].desk, &stan_gry3, sizeof(stan_gry1) - sizeof(long), 0);

          if(!fork()){//proces ktory zrealizuje atak po 5s
            sleep(5);

            semop(sem, &P, 1);
            double obrona = statusy[nr_wroga].ile_lekkiej_piechoty * lekka_piechota.atak;
            obrona += statusy[nr_wroga].ile_ciezkiej_piechoty * ciezka_piechota.atak;
            obrona += statusy[nr_wroga].ile_jazdy * jazda.atak;

            double atak = wiad_atak1.ile_lekkiej_piechoty * lekka_piechota.atak;
            atak += wiad_atak1.ile_ciezkiej_piechoty * ciezka_piechota.atak;
            atak += wiad_atak1.ile_jazdy * jazda.atak;

            if(atak > obrona){//UDANY ATAK
              statusy[nr_gracza].ile_wygranych++;

              if(statusy[nr_gracza].ile_wygranych == 5){//ZWYCIESKI ATAK
                *trwa_gra = false;

                sygnal sygnal1 = {WYGRANA}, sygnal2 = {PRZEGRANA};
                msgsnd(statusy[nr_gracza].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
                msgsnd(statusy[(nr_gracza+1)%3].desk, &sygnal2, sizeof(sygnal) - sizeof(long), 0);
                msgsnd(statusy[(nr_gracza+2)%3].desk, &sygnal2, sizeof(sygnal) - sizeof(long), 0);
                semop(sem, &V, 1);

              } else {//UDANY ATAK, ALE NIE ZWYCIESKI
                statusy[nr_wroga].ile_lekkiej_piechoty = 0;
                statusy[nr_wroga].ile_ciezkiej_piechoty = 0;
                statusy[nr_wroga].ile_jazdy = 0;

                statusy[nr_gracza].ile_lekkiej_piechoty +=  wiad_atak1.ile_lekkiej_piechoty * (1.0 - (obrona/atak));
                statusy[nr_gracza].ile_ciezkiej_piechoty += wiad_atak1.ile_ciezkiej_piechoty * (1.0 - (obrona/atak));
                statusy[nr_gracza].ile_jazdy += wiad_atak1.ile_jazdy * (1.0 - (obrona/atak));

                stan_gry stan_gry1 = generuj_stan(nr_gracza, statusy);
                stan_gry stan_gry2 = generuj_stan(nr_wroga, statusy);
                stan_gry stan_gry3 = generuj_stan(inny_gracz, statusy);
                semop(sem, &V, 1);

                sygnal sygnal1 = {UDANY_ATAK}, sygnal2 = {SUKCES_WROGA}, sygnal3 = {NOWE_DANE};

                msgsnd(statusy[nr_gracza].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
                msgsnd(statusy[nr_gracza].desk, &stan_gry1, sizeof(stan_gry) - sizeof(long), 0);
                msgsnd(statusy[nr_wroga].desk, &sygnal2, sizeof(sygnal) - sizeof(long), 0);
                msgsnd(statusy[nr_wroga].desk, &stan_gry2, sizeof(stan_gry) - sizeof(long), 0);
                msgsnd(statusy[inny_gracz].desk, &sygnal3, sizeof(sygnal) - sizeof(long), 0);
                msgsnd(statusy[inny_gracz].desk, &stan_gry3, sizeof(stan_gry) - sizeof(long), 0);
              }

            } else{//NIEUDANY ATAK
              statusy[nr_wroga].ile_lekkiej_piechoty -= statusy[nr_wroga].ile_lekkiej_piechoty * (atak/obrona);
              statusy[nr_wroga].ile_ciezkiej_piechoty -= statusy[nr_wroga].ile_ciezkiej_piechoty * (atak/obrona);
              statusy[nr_wroga].ile_jazdy -= statusy[nr_wroga].ile_jazdy * (atak/obrona);

              if(obrona > 0){
                statusy[nr_gracza].ile_lekkiej_piechoty +=  wiad_atak1.ile_lekkiej_piechoty * (1.0 - (atak/obrona));
                statusy[nr_gracza].ile_ciezkiej_piechoty += wiad_atak1.ile_ciezkiej_piechoty * (1.0 - (atak/obrona));
                statusy[nr_gracza].ile_jazdy += wiad_atak1.ile_jazdy * (1.0 - (atak/obrona));
              }
              else {
                statusy[nr_gracza].ile_lekkiej_piechoty +=  wiad_atak1.ile_lekkiej_piechoty;
                statusy[nr_gracza].ile_ciezkiej_piechoty += wiad_atak1.ile_ciezkiej_piechoty;
                statusy[nr_gracza].ile_jazdy += wiad_atak1.ile_jazdy;
              }

              stan_gry stan_gry1 = generuj_stan(nr_gracza, statusy);
              stan_gry stan_gry2 = generuj_stan(nr_wroga, statusy);
              stan_gry stan_gry3 = generuj_stan(inny_gracz, statusy);
              semop(sem, &V, 1);


              sygnal sygnal1 = {NIEUDANY_ATAK}, sygnal2 = {PORAZKA_WROGA}, sygnal3 = {NOWE_DANE};

              msgsnd(statusy[nr_gracza].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
              msgsnd(statusy[nr_gracza].desk, &stan_gry1, sizeof(stan_gry) - sizeof(long), 0);
              msgsnd(statusy[nr_wroga].desk, &sygnal2, sizeof(sygnal) - sizeof(long), 0);
              msgsnd(statusy[nr_wroga].desk, &stan_gry2, sizeof(stan_gry) - sizeof(long), 0);
              msgsnd(statusy[inny_gracz].desk, &sygnal3, sizeof(sygnal) - sizeof(long), 0);
              msgsnd(statusy[inny_gracz].desk, &stan_gry3, sizeof(stan_gry) - sizeof(long), 0);
            }

            exit(0);
          }

        } else {//niepoprawne polecenie ataku
          semop(sem, &V, 1);
          sygnal sygnal1 = {ZLA_LICZBA};
          msgsnd(statusy[nr_gracza].desk, &sygnal1, sizeof(sygnal) - sizeof(long), 0);
        }
      }

      //POLACZENIE
      if(!rozlaczenie){
        sygnal sygnal1;
        if(msgrcv(statusy[nr_gracza].desk, &sygnal1, sizeof(sygnal) - sizeof(long), WYJSCIE, IPC_NOWAIT) != -1){
          semop(sem, &P, 1);
          *trwa_gra = false;
          semop(sem, &V, 1);
          powiadom_o_rozlaczeniu(0);
        }
      }
    }

  } else {//proces macierzysty

    //czekanie na zakonczenie procesow
    waitpid(klient1, NULL, 0);
    waitpid(klient2, NULL, 0);
    waitpid(klient3, NULL, 0);

    //sprzatanie
    msgctl(kol_polaczenie, IPC_RMID,0);

    shmdt(ile_graczy);
    shmctl(ile_graczy_id, IPC_RMID, 0);

    shmdt(statusy);
    shmctl(statusy_id, IPC_RMID,0);

    shmdt(trwa_gra);
    shmctl(trwa_gra_id, IPC_RMID, 0);
  }

  return 0;
}
