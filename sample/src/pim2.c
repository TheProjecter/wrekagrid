/*	CE PROGRAMME CALCULE JUSQU'A 150000 (OU 644690 SUIVANT TYPE DE MACHINE)
	DECIMALES DE PI A L'AIDE DE LA FORMULE DE MACHIN:

			PI=16*ARCTG(1/5)-4*ARCTG(1/239)

	CHAQUE ARCTG EST LA SOMME DES Vn AVEC

			Vn=(-1)^n/((2*n+1)*X^(2*n+1))

	X VALANT RESPECTIVEMENT 5 ET 239. CHAQUE TERME Vn EST OBTENU PAR
	RECURRENCE SUIVANT LES FORMULES:

			V0   = 1/X
			Vn+1 = [(-1)*(2*n+1)/((2*n+3)*X*X)]*Vn.

	LES CALCULS SONT FAITS EN MULTIPRECISION SUR 32 BITS (RESP. 64 BITS)
	LA CONTRAINTE DUE A LA LIMITATION DES ENTIERS SUR 32 BITS (RESP. 64
	BITS) FAIT QUE LES CALCULS DES DEUX ARCTG SONT FAITS DIFFEREMMENT
	DANS UN SOUCI D'OPTIMISATION.

	LES CALCULS SONT FAITS SUR DES ENTIERS SIGNES, DONC ON UTILISE
	LES 31 BITS (RESP. 63 BITS) DE CES NOMBRES. LA BASE DE NUMERATION
	UTILISEE EST 1 000 000 (RESP. 10^15) JUSQU'A 1500 (RESP. 6649)
	DECIMALES, 100 000 (RESP. 10^14) JUSQU'A 15 000 (RESP. 66471) ET
	10 000 (RESP. 10^13) JUSQU'A 150 000 (RESP. 644690) DECIMALES CECI
	POUR EVITER DE DEPASSER LES 31 BITS (RESP. 63 BITS) IMPOSES.
	IL Y A DONC UNE BAISSE DE PERFORMANCE AU DELA DE 1500 (RESP. 6649)
	DECIMALES.


	SUR UNE MACHINE A MOTS DE 64 BITS L'ALGORITHME EST PLUS PERFORMANT
	CAR ON UTILISE UNE BASE DE NUMERATION BEAUCOUP PLUS GRANDE.
	A LA COMPILATION IL FAUT DONC CHOISIR SON TYPE DE MACHINE:
	
	cc -DS64BITS -o pim2 pim2.c POUR MACHINE A MOTS DE 64 BITS
	cc -DS32BITS -o pim2 pim2.c POUR MACHINE A MOTS DE 32 BITS

	SI L'ON VEUT TRAVAILLER AVEC DES MOTS DE 32 BITS SUR UNE MACHINE
	A MOTS DE 64 BITS IL FAUT UTILISER L'OPTION DE COMPILATION:
	
	cc -DS32BITSFORCE -o pim2 pim2.c SUR LA MACHINE A MOTS DE 64 BITS.

	CELA PERMET DE COMPARER LES PERFORMANCES DES MACHINES AVEC LE
	MEME ALGORITHME.


		Usage: pim2 [-2] [entier]

			-2:	sortie des decimales en base 2.
			entier:	nombre de decimales maxi.

	DANS LE CAS D'UNE SORTIE DES DECIMALES EN BASE 2, LA PERFORMANCE
	DU PROGRAMME EST MOINS BONNE CAR LE CALCUL EST TOUJOURS FAIT EN
	BASE 10 MAIS ON FAIT ENSUITE UNE CONVERSION EN BASE 2 QUI EST PLUS
	LOURDE QU'UNE CONVERSION EN BASE 10. AVOIR PI EN BASE 2 N'A D'INTERET
	QUE POUR OBTENIR UNE SUITE DE 0 ET DE 1 CONSIDERES COMME ALEATOIRE.

	fichier pim2.c	Andre' Brouty (CNRS / ENST de Bretagne) decembre 1987

		portage pour machine a mots de 64 bits: decembre 1995
		portage pour sortie des decimales en base 2: fevrier 1996

[ http://www-info.enst-bretagne.fr/~brouty/Maths/pi.html ]      */

#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>

#ifdef S64BITS			/* MACHINE CALCULANT SUR 64 BITS */
#define	TypInt	long
#define NBOCT   8		/* NOMBRE D'OCTETS SUR 64 BITS  */
#define BASE1	1000000000000000L	/* BASE DE CALCUL: 10^15        */
#define BASE2	100000000000000L	/* BASE DE CALCUL: 10^14        */
#define BASE3	10000000000000L	/* BASE DE CALCUL: 10^13        */
#define NBZ1	15		/* NOMBRE DE ZEROS DE BASE1     */
#define NBZ2	14		/* NOMBRE DE ZEROS DE BASE2     */
#define NBZ3	13		/* NOMBRE DE ZEROS DE BASE3     */
#define NBIT2	13		/* 2^13 MULTPLICATEUR MINI POUR PASSER EN BASE 2 */
#define GBASE1	4		/* NOMBRE DE BLOCKS DE NBZ1 CHIFFRES AFFICHES PAR LIGNE */
#define GBASE2	4		/* NOMBRE DE BLOCKS DE NBZ2 CHIFFRES AFFICHES PAR LIGNE */
#define GBASE3	5		/* NOMBRE DE BLOCKS DE NBZ3 CHIFFRES AFFICHES PAR LIGNE */
#define LIMP1	6450		/* NOMBRE MAX DE DECIMALES CALCULEES POUR BASE1 */
#define LIMP2	64471		/* NOMBRE MAX DE DECIMALES CALCULEES POUR BASE2 */
#define LIMP3	644691		/* NOMBRE MAX DE DECIMALES CALCULEES POUR BASE3 */
#endif

#ifdef S32BITS			/* MACHINE CALCULANT SUR 32 BITS */
#define	TypInt	long
#define NBOCT   4		/* NOMBRE D'OCTETS SUR 32 BITS  */
#define BASE1	1000000L
#define BASE2	100000L
#define BASE3	10000L
#define NBZ1	6
#define NBZ2	5
#define NBZ3	4
#define NBIT2	11		/* 2^11 MULTPLICATEUR MINI POUR PASSER EN BASE 2 */
#define GBASE1	10		/* NOMBRE DE BLOCKS DE NBZ1 CHIFFRES AFFICHES PAR LIGNE */
#define GBASE2	12		/* NOMBRE DE BLOCKS DE NBZ2 CHIFFRES AFFICHES PAR LIGNE */
#define GBASE3	14		/* NOMBRE DE BLOCKS DE NBZ3 CHIFFRES AFFICHES PAR LIGNE */
#define LIMP1	1501		/* NOMBRE MAX DE DECIMALES CALCULEES POUR BASE1 */
#define LIMP2	15001		/* NOMBRE MAX DE DECIMALES CALCULEES POUR BASE2 */
#define LIMP3	150001		/* NOMBRE MAX DE DECIMALES CALCULEES POUR BASE3 */
#endif

/* ON PEUT FORCER LE CALCUL SUR 32 BITS SUR UNE MACHINE A MOTS DE 64 BITS */
/* AVEC L'OTION DE COMPILATION SUIVANTE. CELA PERMET DE COMPARER LE GAIN  */
/* DE PERFORMANCE QUAND ON UTILISE REELLEMENT LES 64 BITS DE LA MACHINE.  */
/* 	cc -DS32BITSFORCE -o pim2 pim2.c				  */

#ifdef S32BITSFORCE
#define	TypInt	int
#define NBOCT   4		/* NOMBRE D'OCTETS SUR 32 BITS  */
#define BASE1	1000000
#define BASE2	100000
#define BASE3	10000
#define NBZ1	6
#define NBZ2	5
#define NBZ3	4
#define NBIT2	11		/* 2^11 MULTPLICATEUR MINI POUR PASSER EN BASE 2 */
#define GBASE1	10		/* NOMBRE DE BLOCKS DE NBZ1 CHIFFRES AFFICHES PAR LIGNE */
#define GBASE2	12		/* NOMBRE DE BLOCKS DE NBZ2 CHIFFRES AFFICHES PAR LIGNE */
#define GBASE3	14		/* NOMBRE DE BLOCKS DE NBZ3 CHIFFRES AFFICHES PAR LIGNE */
#define LIMP1	1501		/* NOMBRE MAX DE DECIMALES CALCULEES POUR BASE1 */
#define LIMP2	15001		/* NOMBRE MAX DE DECIMALES CALCULEES POUR BASE2 */
#define LIMP3	150001		/* NOMBRE MAX DE DECIMALES CALCULEES POUR BASE3 */
#endif

#define	LOGb10_2 0.30103	/* LOG BASE 10 DE 2 POUR CONVERSION EN BINAIRE */

TypInt *resul;			/* TROIS TABLEAUX DYNAMIQUES CONTENANT RESPECTIVEMENT */
TypInt *resul1;			/* LE RESULTAT FINAL ET LES RESULTATS INTERMEDIAIRES   */
TypInt *resul2;
int K;				/* GESTION DES CASES NULLES: INDICE DE LA 1ERE CASE NON NULLE */
int MAX, MAXB;			/* DIMENSION DES TABLEAUX */
TypInt BASE;			/* BASE DE NUMERATION CORRESPOND AU GROUPEMENT DES CHIFFRES */
int GBASE;			/* NOMBRE DE GROUPEMENT DES CHIFFRES POUR LA SORTIE */
int I, J;			/* NOMBRE D'ITERATIONS POUR CHAQUE ARCTG */

main (argc, argv)
     int argc;
     char *argv[];

{
  int k, i, B2;
  int lim, limb;		/* NOMBRE DE DECIMALES DEMANDEES */
  int NB;
  int Dec;			/* 2^Dec = 1 << Dec */
  char *nam;

  unsigned long mask = 1;
  unsigned int len = sizeof (mask);

  /* This cause the process to stay on the first cpu */
  if (sched_setaffinity (0, len, &mask) < 0) {
    perror ("sched_setaffinity");
  }

  nam = argv[0];
  B2 = 0;
  --argc;
  ++argv;
  lim = 0;
  while (argc) {
    if (*argv[0] == '-') {
      switch (*(argv[0] + 1)) {
      case '2':
	B2 = 1;
	break;
      default:
	Us (nam);
	break;
      }
    } else
      lim = atoi (argv[0]);
    --argc;
    ++argv;
  }
  if (lim == 0) {
    printf ("Nombre de decimales souhaitees ( <= %d): ", LIMP3 - 1);
    scanf ("%d", &lim);
  }
  if (B2) {
    limb = lim;
    lim = (int) ((double) lim * LOGb10_2);
  }
  if (lim < LIMP1) {
    BASE = BASE1;
    NB = NBZ1;
    GBASE = B2 ? 1 + GBASE1 / 2 : GBASE1;
    Dec = NBIT2;
  } else {
    if (lim > LIMP3 - 1) {
      printf ("Pas plus de %d decimales merci.\n", LIMP3 - 1);
      exit (0);
    }
    if (lim < LIMP2) {
      BASE = BASE2;
      NB = NBZ2;
      GBASE = B2 ? GBASE1 / 2 - 1 : GBASE2;
      Dec = NBIT2 + 3;
    } else {
      BASE = BASE3;
      NB = NBZ3;
      GBASE = B2 ? GBASE1 / 2 - 1 : GBASE3;
      Dec = NBIT2 + 6;
    }

  }
  if (B2)
    MAXB = 5 + limb / Dec;
  MAX = 5 + B2 + lim / NB;
  resul = (TypInt *) malloc (NBOCT * MAX);
  resul1 = (TypInt *) malloc (NBOCT * MAX);
  resul2 = (TypInt *) malloc (NBOCT * MAX);
  memset ((char *) resul1, 0, NBOCT * MAX);
  memset ((char *) resul2, 0, NBOCT * MAX);
  memset ((char *) resul, 0, NBOCT * MAX);
  I = arctg_5 (resul1, 5, 16);
  memcpy ((char *) resul2, (char *) resul, NBOCT * MAX);
  memset ((char *) resul, 0, NBOCT * MAX);
#ifdef S64BITS
  if (lim < LIMP1)
    J = arctg_239 (resul1, 239, 4);
  else
    J = arctg_5 (resul1, 239, 4);	/* ON OPTIMISE:239^2 CA PASSE */
#else
  if (lim < LIMP2)
    J = arctg_239 (resul1, 239, 4);
  else
    J = arctg_5 (resul1, 239, 4);	/* ON OPTIMISE:239^2 CA PASSE */
#endif
  K = 2;
  sous (resul2, resul);
  if (B2) {
    printf ("\n Decimales affichees: %d  nombre d'iterations: %d , %d (%d)\n\n",
	    Dec * (MAXB - 4), I, J, I + J);
    tobin (resul2, Dec);
  } else {
    printf ("\n Decimales affichees: %d  nombre d'iterations: %d , %d (%d)\n\n",
	    NB * (MAX - 4), I, J, I + J);
    todec (resul2);
  }
}

todec (tab)			/* CONVERSION EN DECIMAL */
     TypInt tab[];

{
  register i;
  TypInt a;
  TypInt BS10 = BASE / 10;
  printf ("pi = %d,", tab[2]);
  for (i = 3; i < MAX - 1; ++i) {
    a = tab[i];
    a = a == 0 ? 1L : a;
    while (a < BS10) {
      putchar ('0');
      a = 10 * a;
    }
    printf ("%lu%s", tab[i], (i - 2) % GBASE ? " " : "\n       ");
  }
  putchar ('\n');
}

tobin (tab, nb)			/* CONVERSION EN BINAIRE */
     TypInt tab[];
     int nb;

{
  register i;
  TypInt a;
  char *bas2 ();
  a = (TypInt) (1 << (nb - 1));
  printf ("pi= 11,");
  for (i = 3; i < MAXB - 1; ++i) {
    tab[0] = 0;
    tab[1] = 0;
    tab[2] = 0;
    mul2 (tab, nb);
    if (tab[1])
      tab[2] = BASE * BASE * tab[0] + BASE * tab[1] + tab[2];
    printf ("%s%s", bas2 (tab[2], a, nb), (i - 2) % GBASE ? " " : "\n       ");
  }
  putchar ('\n');
}

char *
bas2 (b, p, n)			/* AFFICHE LES CHIFFRES BINAIRES DE b PAR BLOC DE n CHIFFRES */
     TypInt b, p;
     int n;

{
  short i;
  static char Base[256];
  memset (Base, 0, 256);
  for (i = 0; i < n; ++i) {
    Base[i] = (b & (p >> i)) > 0 ? '1' : '0';
  }
  return (Base);
}

add (a, b)			/* ADDITION DE DEUX TABLEAUX DE NOMBRES */
     TypInt a[], b[];

{
  register i;
  TypInt register r = 0;	/* r EST LA RETENUE */
  for (i = MAX - 1; i >= K; --i) {
    r = a[i] + r + b[i];
    if (r >= BASE) {
      a[i] = r - BASE;
      r = 1;
    } else {
      a[i] = r;
      r = 0;
    }
  }
  while (r) {			/* PROPAGATION DE LA RETENUE */
    r = a[i] + r;
    if (r >= BASE) {
      a[i] = r - BASE;
      r = 1;
    } else {
      a[i] = r;
      r = 0;
    }
    --i;
  }
}

sous (a, b)			/* SOUSTRACTION DE DEUX TABLEAUX DE NOMBRES. LE SIGNE DU */
     TypInt a[], b[];		/* RESULTAT EST SUPPOSE POSITIF                          */

{
  register i;
  TypInt register r = 0;
  for (i = MAX - 1; i >= K; --i) {
    r = a[i] - r - b[i];
    if (r < 0) {
      a[i] = r + BASE;
      r = 1;
    } else {
      a[i] = r;
      r = 0;
    }
  }
  while (r) {			/* PROPAGATION DE LA RETENUE */
    r = a[i] - r;
    if (r < 0) {
      a[i] = r + BASE;
      r = 1;
    } else {
      a[i] = r;
      r = 0;
    }
    --i;
  }
}

mul (tab, n)			/* MULTIPLICATION D'UN TABLEAU DE NOMBRES PAR UN ENTIER POSITIF */
     TypInt tab[], n;

{
  register i;
  TypInt register S = 0;
  for (i = MAX - 1; i >= K; --i) {
    tab[i] = tab[i] * n + S;
    S = tab[i] / BASE;
    tab[i] = tab[i] % BASE;
  }
  tab[i] = tab[i] * n + S;
  while ((S = tab[i]) >= BASE) {
    tab[i - 1] = tab[i - 1] + S / BASE;
    tab[i] = S % BASE;
    --i;
    --K;
  }
  --K;
  while (tab[K] == 0)
    ++K;
}

mul2 (tab, n)			/* MULTIPLICATION D'UN TABLEAU DE NOMBRES PAR 2^n */
     TypInt tab[], n;

{
  register i;
  TypInt register S = 0;
  for (i = MAX - 1; i >= K; --i) {
    tab[i] = (tab[i] << n) + S;
    S = tab[i] / BASE;
    tab[i] = tab[i] % BASE;
  }
  tab[i] = (tab[i] << n) + S;
  while ((S = tab[i]) >= BASE) {
    tab[i - 1] = tab[i - 1] + S / BASE;
    tab[i] = S % BASE;
    --i;
    --K;
  }
  --K;
  while (tab[K] == 0)
    ++K;
}

divisi (t, n)			/* DIVISION D'UN TABLEAU DE NOMBRES PAR UN ENTIER */
     TypInt t[], n;

{
  register i;
  TypInt register a;
  a = t[K];
  for (i = K; i < MAX; ++i) {
    t[i] = a / n;
    a = BASE * (a % n) + t[i + 1];
  }
  while (t[K] == 0)
    ++K;			/* GESTION DES CASES NULLES */
}

arctg_239 (tab, p, b)		/* b*arctg(1/p) POUR p DE L'ORDRE DE GRANDEUR DE 239 */
     TypInt tab[], p, b;

{
  register i;
  K = 2;
  tab[2] = b;
  divisi (tab, p);
  add (resul, tab);
  for (i = 0; K < MAX; ++i) {
    divisi (tab, (2 * i + 3));
    divisi (tab, p);		/*ON FAIT 2 DIVISIONS SUCCESSIVES AU LIEU D'UNE */
    divisi (tab, p);		/*POUR EVITER DE DEPASSER LES 31 (OU 63) BITS  */
    mul (tab, (2 * i + 1));
    (i + 1) % 2 ? sous (resul, tab) : add (resul, tab);
  }
  return (i);
}

arctg_5 (tab, p, b)		/* b*arctg(1/p) POUR p DE L'ORDRE DE GRANDEUR DE 5 */
     TypInt tab[], p, b;

{
  register i;
  K = 2;
  tab[2] = b;
  divisi (tab, p);
  add (resul, tab);
  p = p * p;
  for (i = 0; K < MAX; ++i) {
    divisi (tab, (2 * i + 3));
    divisi (tab, p);		/*ICI p EST TROP PETIT POUR RISQUER DE DEPASSER */
    mul (tab, (2 * i + 1));	/*       LES 31 (OU 63) BITS.         */
    (i + 1) % 2 ? sous (resul, tab) : add (resul, tab);
  }
  return (i);
}

Us (nom)			/* USAGE DE LA COMMANDE */
     char *nom;

{
  printf
    ("Usage: %s [-2] [entier]\n\n\t-2: sortie en base 2\n\tentier: nombre de decimales desirees.\n",
     nom);
  exit (1);
}
