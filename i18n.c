/*
 * i18n.c: Internationalization
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: i18n.c 1.66 2002/03/17 13:50:43 kls Exp $
 *
 * Slovenian translations provided by Miha Setina <mihasetina@softhome.net>
 * Italian   translations provided by Alberto Carraro <bertocar@tin.it>
 * Dutch     translations provided by Arnold Niessen <niessen@iae.nl> <arnold.niessen@philips.com>
 * Portugese translations provided by Paulo Manuel Martins Lopes <pmml@netvita.pt>
 * French    translations provided by Jean-Claude Repetto <jc@repetto.org>
 * Norwegian translations provided by J�rgen Tvedt <pjtvedt@online.no>
 * Finnish   translations provided by Hannu Savolainen <hannu@opensound.com>
 *
 */

/*
 * How to add a new language:
 *
 * 1. Announce your translation action on the Linux-DVB mailing
 *    list to avoid duplicate work.
 * 2. Increase the value of 'NumLanguages'.
 * 3. Insert a new line in every member of the 'Phrases[]' array,
 *    containing the translated text for the new language.
 *    For example, assuming you want to add the Italian language,
 *
 *       { "English",
 *         "Deutsch",
 *       },
 *
 *    would become
 *
 *       { "English",
 *         "Deutsch",
 *         "Italiano",
 *       },
 *
 *    and so on. Append your language after the last existing language
 *    and write the name of your language in your language (not in English,
 *    which means that it should be 'Italiano', not 'Italian').
 *    Note that only the characters defined in 'fontosd.c' will
 *    be available!
 * 4. Compile VDR and test the new language by switching to it
 *    in the "Setup" menu.
 * 5. Send the modified 'i18n.c' file to <kls@cadsoft.de> to have
 *    it included in the next version of VDR.
 *
 * In case an English phrase is used in more than one context (and might need
 * different translations in other languages) it can be preceeded with an
 * arbitrary string to describe its context, separated from the actual phrase
 * by a '$' character (see for instance "Button$Stop" vs. "Stop").
 * Of course this means that no English phrase may contain the '$' character!
 * If this should ever become necessary, the existing '$' would have to be
 * replaced with something different...
 */

#include "i18n.h"
#include <stdio.h>
#include "config.h"
#include "tools.h"

const int NumLanguages = 9;

typedef const char *tPhrase[NumLanguages];

const tPhrase Phrases[] = {
  // The name of the language (this MUST be the first phrase!):
  { "English",
    "Deutsch",
    "Slovenski",
    "Italiano",
    "Nederlands",
    "Portugues",
    "Fran�ais",
    "Norsk",
    "Suomi",
  },
  // Menu titles:
  { "VDR",
    "VDR",
    "VDR",
    "VDR",
    "VDR",
    "VDR",
    "VDR",
    "VDR",
    "VDR",
  },
  { "Schedule",
    "Programm",
    "Urnik",
    "Programmi",
    "Gids",
    "Programa",
    "Programmes",
    "Programmer",
    "Ohjelmat",
  },
  { "Channels",
    "Kan�le",
    "Kanali",
    "Canali",
    "Kanalen",
    "Canal",
    "Cha�nes",
    "Kanaler",
    "Kanavat",
  },
  { "Timers",
    "Timer",
    "Termini",
    "Timer",
    "Timers",
    "Alarmes",
    "Programmation",
    "Timer",
    "Ajastin",
  },
  { "Recordings",
    "Aufzeichnungen",
    "Posnetki",
    "Registrazioni",
    "Opnames",
    "Gravacoes",
    "Enregistrements",
    "Opptak",
    "Nauhoitteet",
  },
  { "Setup",
    "Einstellungen",
    "Nastavitve",
    "Opzioni",
    "Instellingen",
    "Configurar",
    "Configuration",
    "Konfigurasjon",
    "Asetukset",
  },
  { "Commands",
    "Befehle",
    "Ukazi",
    "Comandi",
    "Commando's",
    "Comandos",
    "Commandes",
    "Kommandoer",
    "Komennot",
  },
  { "Edit Channel",
    "Kanal Editieren",
    "Uredi kanal",
    "Modifica canale",
    "Kanaal aanpassen",
    "Modificar Canal",
    "Modifier une cha�ne",
    "Editer Kanal",
    "Muokkaa kanavaa",
  },
  { "Edit Timer",
    "Timer Editieren",
    "Uredi termin",
    "Modifica Timer",
    "Timer veranderen",
    "Modificar Alarme",
    "Changer la programmation",
    "Editer Timer",
    "Muokkaa ajastusta",
  },
  { "Event",
    "Sendung",
    "Oddaja",
    "Eventi",
    "Uitzending",
    "Evento",
    "Ev�nement",
    "Hendelse",
    "Tapahtuma",
  },
  { "Summary",
    "Inhalt",
    "Vsebina",
    "Sommario",
    "Inhoud",
    "Resumo",
    "R�sum�",
    "Sammendrag",
    "Yhteenveto",
  },
  { "Schedule - %s",
    "Programm - %s",
    "Urnik - %s",
    "Programma - %s",
    "Programma - %s",
    "Programa - %s",
    "Programmes - %s",
    "Program Guide - %s",
    "Ohjelma - %s",
  },
  { "What's on now?",
    "Was l�uft jetzt?",
    "Kaj je na sporedu?",
    "In programmazione",
    "Wat is er nu?",
    "O que ver agora?",
    "Programmes en cours",
    "Hvilket program sendes n�?",
    "Nykyinen ohjelma",
  },
  { "What's on next?",
    "Was l�uft als n�chstes?",
    "Kaj sledi?",
    "Prossimi programmi",
    "Wat komt er hierna?",
    "O que ver depois?",
    "Prochains programmes",
    "Hvilket program er neste?",
    "Seuraava ohjelma",
  },
  // Button texts (should not be more than 10 characters!):
  { "Edit",
    "Editieren",
    "Uredi",
    "Modifica",
    "Verander",
    "Modificar",
    "Modifier",
    "Editer",
    "Muuta",
  },
  { "New",
    "Neu",
    "Novo",
    "Nuovo",
    "Nieuw",
    "Novo",
    "Nouveau",
    "Ny",
    "Uusi",
  },
  { "Delete",
    "L�schen",
    "Odstrani",
    "Cancella",
    "Verwijder",
    "Apagar",
    "Supprimer",
    "Slett",
    "Poista",
  },
  { "Mark",
    "Markieren",
    "Oznaci",
    "Marca",
    "Verplaats",
    "Marcar",
    "Marquer",
    "Marker",
    "Merkitse",
  },
  { "On/Off",
    "Ein/Aus",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Marche/Arr",
    "", // TODO
    "", // TODO
  },
  { "Record",
    "Aufnehmen",
    "Posnemi",
    "Registra",
    "Opnemen",
    "Gravar",
    "Enregistre",
    "Ta opp",
    "Nauhoita",
  },
  { "Play",
    "Wiedergabe",
    "Predavajaj",
    "Riproduci",
    "Afspelen",
    "Play",
    "Lire",
    "Spill av",
    "Toista",
  },
  { "Rewind",
    "Anfang",
    "Zacetek",
    "Da inizio",
    "Spoel terug",
    "Rebobinar",
    "Retour",
    "Spol tilbake",
    "Takaisinkel.",
  },
  { "Button$Stop",
    "Beenden",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  { "Resume",
    "Weiter",
    "Nadaljuj",
    "Riprendi",
    "Verder",
    "Continuar",
    "Reprendre",
    "Fortsett",
    "Jatka",
  },
  { "Summary",
    "Inhalt",
    "Vsebina",
    "Sommario",
    "Inhoud",
    "Resumo",
    "R�sum�",
    "Sammendrag",
    "Yhteenveto",
  },
  { "Open",
    "�ffnen",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Ouvrir",
    "", // TODO
    "Avaa",
  },
  { "Switch",
    "Umschalten",
    "Preklopi",
    "Cambia",
    "Selecteer",
    "Seleccionar",
    "Regarder",
    "Skift til",
    "Valitse",
  },
  { "Now",
    "Jetzt",
    "Sedaj",
    "Adesso",
    "Nu",
    "Agora",
    "Maintenant",
    "N�",
    "Nyt",
  },
  { "Next",
    "N�chste",
    "Naslednji",
    "Prossimo",
    "Hierna",
    "Proximo",
    "Apr�s",
    "Neste",
    "Seuraava",
  },
  { "Button$Schedule",
    "Programm",
    "Urnik",
    "Programma",
    "Programma",
    "Programa",
    "Programme",
    "Programmer",
    "Ohjelmisto",
  },
  { "Language",
    "Sprache",
    "Jezik",
    "Linguaggio",
    "Taal",
    "Linguagem", // TODO (correct?)
    "Langue",
    "Spr�k",
    "Kieli",
  },
  { "Eject",
    "Auswerfen",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Ejection",
    "", // TODO
    "Avaa",
  },
  // Confirmations:
  { "Delete channel?",
    "Kanal l�schen?",
    "Odstrani kanal?",
    "Cancello il canale?",
    "Kanaal verwijderen?",
    "Apagar o Canal?",
    "Supprimer la cha�ne?",
    "Slette kanal?",
    "Poistetaanko kanava?",
  },
  { "Delete timer?",
    "Timer l�schen?",
    "Odstani termin?",
    "Cancello il timer?",
    "Timer verwijderen?",
    "Apagar o Alarme?",
    "Supprimer la programmation?",
    "Slette timer?",
    "Poistetaanko ajastus?",
  },
  { "Delete recording?",
    "Aufzeichnung l�schen?",
    "Odstrani posnetek?",
    "Cancello la registrazione?",
    "Opname verwijderen?",
    "Apagar Gravac�o?",
    "Supprimer l'enregistrement?",
    "Slette opptak?",
    "Poistetaanko nauhoitus?",
  },
  { "Timer still recording - really delete?",
    "Timer zeichnet auf - trotzdem l�schen?",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Enregistrement en cours - confirmez la suppression",
    "", // TODO
    "", // TODO
  },
  { "Stop recording?",
    "Aufzeichnung beenden?",
    "Koncaj snemanje?",
    "Fermo la registrazione?",
    "Opname stoppen?",
    "Parar Gravac�o?",
    "Arr�ter l'enregistrement?",
    "Stoppe opptak?",
    "Pys�ytet��nk� nauhoitus?",
  },
  { "on primary interface",
    "auf dem prim�ren Interface",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "sur la carte principale",
    "", // TODO
    "p��vastaanottimella",
  },
  { "Cancel editing?",
    "Schneiden abbrechen?",
    "Zelite prekiniti urejanje?",
    "Annullo la modifica?",
    "Bewerken afbreken?",
    "Cancelar Modificar?",
    "Annuler les modifications?",
    "Avbryte redigering?",
    "Peruutetaanko muokkaus?",
  },
  { "Really restart?",
    "Wirklich neu starten?",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  { "Recording - restart anyway?",
    "Aufnahme l�uft - trotzdem neu starten?",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  { "Recording - shut down anyway?",
    "Aufnahme l�uft - trotzdem ausschalten?",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Enregistrement en cours - confirmez l'arr�t",
    "", // TODO
    "Nauhoitus kesken - lopetetaanko se?",
  },
  { "Recording in %d minutes, shut down anyway?",
    "Aufnahme in %d Minuten - trotzdem ausschalten?",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Enregistrement dans %d minutes - confirmez l'arr�t", // TODO
    "", // TODO
    "", // TODO
  },
  { "Press any key to cancel shutdown",
    "Taste dr�cken um Shutdown abzubrechen",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Appuyez sur une touche pour annuler l'arr�t",
    "", // TODO
    "Peruuta pys�ytys painamalla jotakin n�pp�int�",
  },
  // Channel parameters:
  { "Name",
    "Name",
    "Naziv",
    "Nome",
    "Naam",
    "Nome",
    "Nom",
    "Navn",
    "Nimi",
  },
  { "Frequency",
    "Frequenz",
    "Frekvenca",
    "Frequenza",
    "Frequentie",
    "Frequencia",
    "Fr�quence",
    "Frekvens",
    "Taajuus",
  },
  { "Polarization",
    "Polarisation",
    "Polarizacija",
    "Polarizzazione",
    "Polarisatie",
    "Polarizacao",
    "Polarisation",
    "Polaritet",
    "Polarisaatio",
  },
  { "DiSEqC",
    "DiSEqC",
    "DiSEqC",
    "DiSEqC",
    "DiSEqC",
    "DiSEqC",
    "DiSEqC",
    "DiSEqC",
    "DiSEqC",
  },
  { "Srate",
    "Srate",
    "Srate",
    "Srate",
    "Srate",
    "Srate",
    "Fr�q. Symbole",
    "Symbolrate",
    "Srate",
  },
  { "Vpid",
    "Vpid",
    "Vpid",
    "Vpid",
    "Vpid",
    "Vpid",
    "PID Vid�o",
    "Video pid",
    "Kuva PID",
  },
  { "Apid1",
    "Apid1",
    "Apid1",
    "Apid1",
    "Apid1",
    "Apid1",
    "PID Audio (1)",
    "Audio pid1",
    "��ni PID1",
  },
  { "Apid2",
    "Apid2",
    "Apid2",
    "Apid2",
    "Apid2",
    "Apid2",
    "PID Audio (2)",
    "Audio pid2",
    "��ni PID2",
  },
  { "Dpid1",
    "Dpid1",
    "Dpid1",
    "Dpid1",
    "Dpid1",
    "Dpid1",
    "PID AC3 (1)",
    "AC3 pid1",
    "AC3 PID1",
  },
  { "Dpid2",
    "Dpid2",
    "Dpid2",
    "Dpid2",
    "Dpid2",
    "Dpid2",
    "PID AC3 (2)",
    "AC3 pid2",
    "AC3 PID2",
  },
  { "Tpid",
    "Tpid",
    "Tpid",
    "Tpid",
    "Tpid",
    "Tpid",
    "PID T�l�texte",
    "Teletext pid",
    "TekstiTV PID",
  },
  { "CA",
    "CA",
    "CA",
    "CA",
    "CA",
    "CA",
    "Cryptage",
    "Kortleser",
    "Salauskortti",
  },
  { "Pnr",
    "Pnr",
    "Pnr",
    "Pnr",
    "Pnr",
    "Pnr",
    "Num. Progr.",
    "Program Id",
    "Ohjelmatunnus",
  },
  // Timer parameters:
  { "Active",
    "Aktiv",
    "Aktivno",
    "Attivo",
    "Actief",
    "Activo",
    "Actif",
    "Aktiv",
    "Aktiivinen",
  },
  { "Channel",
    "Kanal",
    "Kanal",
    "Canale",
    "Kanaal",
    "Canal",
    "Cha�ne",
    "Kanal",
    "Kanava",
  },
  { "Day",
    "Tag",
    "Dan",
    "Giorno",
    "Dag",
    "Dia",
    "Jour",
    "Dag",
    "P�iv�",
  },
  { "Start",
    "Anfang",
    "Zacetek",
    "Inizio",
    "Begin",
    "Inicio",
    "D�but",
    "Start",
    "Aloitus",
  },
  { "Stop",
    "Ende",
    "Konec",
    "Fine",
    "Einde",
    "Fim",
    "Fin",
    "Slutt",
    "Lopetus",
  },
  { "Priority",
    "Priorit�t",
    "Prioriteta",
    "Priorita",
    "Prioriteit",
    "Prioridade",
    "Priorit�",
    "Prioritet",
    "Prioriteetti",
  },
  { "Lifetime",
    "Lebensdauer",
    "Veljavnost",
    "Durata",
    "Bewaarduur",
    "Duracao",
    "Dur�e de vie",
    "Levetid",
    "Voimassaolo",
  },
  { "File",
    "Datei",
    "Datoteka",
    "Nome",
    "Filenaam",
    "Ficheiro",
    "Fichier",
    "Filnavn",
    "Tiedosto",
  },
  { "First day",
    "Erster Tag",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Premier jour",
    "", // TODO
    "", // TODO
  },
  // Error messages:
  { "Channel is being used by a timer!",
    "Kanal wird von einem Timer benutzt!",
    "Urnik zaseda kanal!",
    "Canale occupato da un timer!",
    "Kanaal wordt gebruikt door een timer!",
    "Canal a ser utilizador por um alarme!",
    "Cette cha�ne est en cours d'utilisation!",
    "Kanalen er i bruk av en timer!",
    "Kanava on ajastimen k�yt�ss�!",
  },
  { "Can't switch channel!",
    "Kanal kann nicht umgeschaltet werden!",
    "Ne morem preklopiti kanala!",
    "Impossibile cambiare canale!",
    "Kan geen kanaal wisselen!",
    "Nao pode mudar de canal!",
    "Impossible de changer de cha�ne!",
    "Ikke mulig � skifte kanal!",
    "Kanavan vaihtaminen ei mahdollista!",
  },
  { "Timer is recording!",
    "Timer zeichnet gerade auf!",
    "Snemanje po urniku!",
    "Registrazione di un timer in corso!",
    "Timer is aan het opnemen!",
    "Alarme a gravar!",
    "Enregistrement en cours!",
    "Timer gj�r opptak!",
    "Ajastinnauhoitus k�ynniss�!",
  },
  { "Error while accessing recording!",
    "Fehler beim ansprechen der Aufzeichnung!",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Impossible d'acc�der � l'enregistrement",
    "", // TODO
    "Nauhoituksen toistaminen ep�onnistui!",
  },
  { "Error while deleting recording!",
    "Fehler beim L�schen der Aufzeichnung!",
    "Napaka pri odstranjevanju posnetka!",
    "Errore durante la canc del filmato!",
    "Fout bij verwijderen opname!",
    "Erro enquanto apagava uma gravacao!",
    "Erreur de suppression de l'enregistrement!",
    "Feil under sletting av opptak!",
    "Nauhoituksen poistaminen ep�onnistui!",
  },
  { "*** Invalid Channel ***",
    "*** Ung�ltiger Kanal ***",
    "*** Neznan kanal ***",
    "*** CANALE INVALIDO ***",
    "*** Ongeldig kanaal ***",
    "*** Canal Invalido! ***",
    "*** Cha�ne invalide! ***",
    "*** Ugyldig Kanal! ***",
    "*** Virheellinen kanavavalinta! ***",
  },
  { "No free DVB device to record!",
    "Keine freie DVB-Karte zum Aufnehmen!",
    "Ni proste DVB naprave za snemanje!",
    "Nessuna card DVB disp per registrare!",
    "Geen vrije DVB kaart om op te nemen!",
    "Nenhuma placa DVB disponivel para gravar!",
    "Pas de carte DVB disponible pour l'enregistrement!",
    "Ingen ledige DVB enheter for opptak!",
    "Ei vapaata vastaanotinta nauhoitusta varten!",
  },
  { "Channel locked (recording)!",
    "Kanal blockiert (zeichnet auf)!",
    "Zaklenjen kanal (snemanje)!",
    "Canale bloccato (in registrazione)!",
    "Kanaal geblokkeerd (neemt op)!",
    "Canal bloqueado (a gravar)!",
    "Cha�ne verrouill�e (enregistrement en cours)!",
    "Kanalen er l�st (opptak)!",
    "Kanava lukittu (nauhoitusta varten)!",
  },
  { "Can't start Transfer Mode!",
    "Transfer-Mode kann nicht gestartet werden!",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Impossible d'utiliser le mode transfert!",
    "", // TODO
    "K�sitt�m�tt�mi� teknisi� ongelmia!",
  },
  { "Can't start editing process!",
    "Schnitt kann nicht gestartet werden!",
    "Ne morem zaceti urejanja!",
    "Imposs iniziare processo di modifica",
    "Kan niet beginnen met bewerken!",
    "Nao pode iniciar a modificacao!",
    "Impossible de commencer le montage!",
    "Kan ikke starte redigeringsprosessen!",
    "Muokkauksen aloittaminen ei onnistu!",
  },
  { "Editing process already active!",
    "Schnitt bereits aktiv!",
    "Urejanje je ze aktivno!",
    "Processo di modifica gia` attivo",
    "Bewerken is al actief!",
    "Processo de modificacao ja activo!",
    "Montage d�j� en cours!",
    "Redigeringsprosessen er allerede aktiv!",
    "Muokkaus on jo k�ynniss�!",
  },
  { "Can't shutdown - option '-s' not given!",
    "Shutdown unm�glich - Option '-s' fehlt!",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Arr�t impossible - option '-s' absente!",
    "", // TODO
    "Ei voida sammuttaa '-s' parametria ei annettu!",
  },
  { "Low disk space!",
    "Platte beinahe voll!",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Disque presque plein!",
    "", // TODO
    "Kovalevy l�hes t�ynn�!",
  },
  // Setup pages:
  { "OSD",
    "OSD",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  { "EPG",
    "EPG",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  { "DVB",
    "DVB",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  { "LNB",
    "LNB",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  { "CICAM",
    "CICAM",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  { "Recording",
    "Aufnahme",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  { "Replay",
    "Wiedergabe",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  { "Miscellaneous",
    "Sonstiges",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  { "Restart",
    "Neustart",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  // Setup parameters:
  { "Setup.OSD$Language",
    "Sprache",
    "Jezik",
    "Linguaggio",
    "Taal",
    "Linguagem", // TODO (correct?)
    "Langue",
    "Spr�k",
    "Kieli",
  },
  { "Setup.OSD$Width",
    "Breite",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Largeur", //TODO (correct?)
    "", // TODO
    "Leveys", //TODO (correct?)
  },
  { "Setup.OSD$Height",
    "H�he",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Hauteur", //TODO (correct?)
    "", // TODO
    "Korkeus", //TODO (correct?)
  },
  { "Setup.OSD$Message time",
    "Anzeigedauer f�r Nachrichten",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Dur�e affichage �cran",
    "", // TODO
    "Ilmoitusten n�kymisaika",
  },
  { "Setup.OSD$Channel info position",
    "Kanal-Info Position",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Position infos cha�nes",
    "", // TODO
    "Kanavainfon sijainti",
  },
  { "Setup.OSD$Info on channel switch",
    "Info beim Kanalwechsel",
    "Pokazi naziv kanala",
    "Vis info nel cambio canale",
    "Kanaal info tonen",
    "Mostrar info ao mudar de Canal",
    "Affichage progr. en cours",
    "Info ved kanalskifte",
    "N�yt� kanavainfo",
  },
  { "Setup.OSD$Scroll pages",
    "Seitenweise scrollen",
    "Drsni meni",
    "Scrolla pagina nel menu",
    "Scrollen per pagina",
    "Scroll da pagina no menu",
    "Affichage progr. suivant",
    "Rask rulling i menyer",
    "Valikkojen rullaus",
  },
  { "Setup.OSD$Sort timers",
    "Timer sortieren",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Trier les programmations",
    "", // TODO
    "J�rjest� ajastimet",
  },
  { "Setup.OSD$Recording directories",
    "Aufnahme Verzeichnisse",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Dossiers d'enregistrements",
    "", // TODO
    "Nauhoitushakemistot",
  },
  { "Setup.EPG$EPG scan timeout",
    "Zeit bis EPG Scan",
    "Cas do EPG pregleda",
    "Timeout EPG",
    "EPG-scan Timeout",
    "Timeout EPG",
    "Temps maxi EPG",
    "Ledig tid f�r EPG-s�k",
    "Ohjelmatied. odotusaika",
  },
  { "Setup.EPG$EPG bugfix level",
    "EPG Fehlerbereinigung",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Niveau de correction EPG",
    "", // TODO
    "EPG Bugfix Level",
  },
  { "Setup.EPG$Set system time",
    "Systemzeit stellen",
    "Sistemski cas",
    "Setta orario auto",
    "Systeem klok instellen",
    "Ajustar relogio do sistema",
    "Ajuster l'heure du syst�me",
    "Juster system-klokken",
    "Vastaanota kellonaika",
  },
  { "Setup.EPG$Use time from transponder",
    "Transponder f�r Systemzeit",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
  },
  { "Setup.DVB$Primary DVB interface",
    "Prim�res DVB Interface",
    "Primarna naprava",
    "Scheda DVB primaria",
    "Eerste DVB kaart",
    "DVB primario",
    "Premi�re carte DVB",
    "Hoved DVB-enhet",
    "Ensisij. vast.otin",
  },
  { "Setup.DVB$Video format",
    "Video Format",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Format vid�o",
    "TV Format",
    "Kuvamuoto",
  },
  { "Setup.LNB$SLOF",
    "SLOF",
    "SLOF",
    "SLOF",
    "SLOF",
    "SLOF",
    "Limite de bandes LNB",
    "LO-grensefrekvens",
    "SLOF",
  },
  { "Setup.LNB$Low LNB frequency",
    "Untere LNB-Frequenz",
    "Spodnja LNB-frek.",
    "Freq LO LNB",
    "Laagste LNB frequentie",
    "Freq LO LNB",
    "Fr�quence basse LNB",
    "LO-frekvens i lavb�ndet",
    "LO LNB taajuus",
  },
  { "Setup.LNB$High LNB frequency",
    "Obere LNB-Frequenz",
    "Zgornja LNB-frek.",
    "Freq HI LNB",
    "Hoogste LNB frequentie",
    "Freq HI LNB",
    "Fr�quence haute LNB",
    "LO-frekvens i h�yb�ndet",
    "HI LNB taajuus",
  },
  { "Setup.LNB$Use DiSEqC",
    "DiSEqC benutzen",
    "DiSEqC", // TODO
    "DiSEqC", // TODO
    "DiSEqC", // TODO
    "DiSEqC", // TODO
    "DiSEqC", // TODO
    "DiSEqC", // TODO
    "DiSEqC", // TODO
  },
  { "Setup.CICAM$CICAM DVB",
    "CICAM DVB",
    "CICAM DVB",
    "CICAM DVB",
    "CICAM DVB",
    "CICAM DVB",
    "CICAM DVB",
    "CICAM DVB",
    "CICAM DVB",
  },
  { "Setup.Recording$Margin at start",
    "Zeitpuffer bei Anfang",
    "Premor pred zacetkom",
    "Min margine inizio",
    "Tijd marge (begin)",
    "Margem de inicio",
    "Marge ant�rieure",
    "Opptaks margin (start)",
    "Aloitusmarginaali",
  },
  { "Setup.Recording$Margin at stop",
    "Zeitpuffer bei Ende",
    "Premor za koncem",
    "Min margine fine",
    "Tijd marge (eind)",
    "Margem de fim",
    "Marge post�rieure",
    "Opptaks margin (slutt)",
    "Lopetusmarginaali",
  },
  { "Setup.Recording$Primary limit",
    "Prim�r-Limit",
    "", // TODO
    "", // TODO
    "", // TODO
    "Limite Primario",
    "Premi�re limite",
    "Prioritets grense HovedDVB",
    "PrimaryLimit",
  },
  { "Setup.Recording$Default priority",
    "Default Priorit�t",
    "", // TODO
    "", // TODO
    "", // TODO
    "Prioridade por defeito",
    "Priorit� par d�faut",
    "Normal prioritet (Timer)",
    "Oletusprioriteetti",
  },
  { "Setup.Recording$Default lifetime",
    "Default Lebensdauer",
    "", // TODO
    "", // TODO
    "", // TODO
    "Validade por defeito",
    "Dur�e de vie par d�faut",
    "Normal levetid (Timer)",
    "Oletus voimassaoloaika",
  },
  { "Setup.Recording$Use episode name",
    "Episodenname verwenden",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Utiliser les sous-titres", // TODO (episode name!)
    "", // TODO
    "Tekstitys k�yt�ss�", // TODO (episode name!)
  },
  { "Setup.Recording$Mark instant recording",
    "Direktaufzeichnung markieren",
    "Oznaci direktno snemanje",
    "Marca la registrazione",
    "Direkte opnamen markeren",
    "Marca de gravacao",
    "Enregistrement imm�diat",
    "Markere direkteopptak",
    "Merkitse v�lit�n nauh.",
  },
  { "Setup.Recording$Name instant recording",
    "Direktaufzeichnung benennen",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Noms enregistr. imm�diats",
    "", // TODO
    "Nime� v�lit�n nauh.",
  },
  { "Setup.Recording$Record Dolby Digital",
    "Dolby Digital Ton aufzeichnen",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Enregistrer en Dolby Digital", 
    "", // TODO
    "", // TODO
  },
  { "Setup.Recording$Max. video file size",
    "Max. Video Dateigr��e",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Taille maxi des fichiers",
    "", // TODO
    "Maksimi tiedoston koko",
  },
  { "Setup.Recording$Split edited files",
    "Editierte Dateien aufteilen",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "S�parer les s�quences",
    "", // TODO
    "Paloittele muokatut",
  },
  { "Setup.Replay$Multi speed mode",
    "MultiSpeed Modus",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Mode multi-vitesses",
    "", // TODO
    "Moninopeustila",
  },
  { "Setup.Replay$Show replay mode",
    "Wiedergabestatus anzeigen",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Affichage mode de lecture",
    "", // TODO
    "N�yt� toiston tila",
  },
  { "Setup.Miscellaneous$Min. event timeout",
    "Mindest Event Pause",
    "", // TODO
    "", // TODO
    "", // TODO
        "", // TODO
    "MinEventTimeout", // Too difficult to translate - read the manual!
    "", // TODO
    "Minimi tapahtuman odotus",
  },
  { "Setup.Miscellaneous$Min. user inactivity",
    "Mindest Benutzer-Inaktivit�t",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Dur�e minimale d'inactivit�",
    "", // TODO
    "Minimi k�ytt�j�n odotus",
  },
  { "Setup.Miscellaneous$SVDRP timeout",
    "SVDRP Timeout",
    "", // TODO
    "Timeout SVDRP",
    "SVDRP Timeout",
    "Timeout SVDRP",
    "Temps maxi SVDRP",
    "Ubrukt SVDRP-levetid",
    "SVDRP odotusaika",
  },
  // The days of the week:
  { "MTWTFSS",
    "MDMDFSS",
    "PTSCPSN",
    "DLMMGVS",
    "MDWDVZZ",
    "STQQSSD",
    "LMMJVSD",
    "MTOTFLS",
    "MTKTPLS",
  },
  { "MonTueWedThuFriSatSun", // must all be 3 letters!
    "MonDieMitDonFreSamSon",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "LunMarMerJeuVenSamDim",
    "", // TODO
    "MaaTiiKesTorPerLauSun",
  },
  // Learning keys:
  { "Learning Remote Control Keys",
    "Fernbedienungs-Codes lernen",
    "Ucim se kod upravljalca",
    "Apprendimento tasti unita` remota",
    "Leren toetsen afstandsbediening",
    "Aprender as teclas do telecomando",
    "Apprentissage des codes de t�l�commande",
    "L�re fjernkontrolltaster",
    "Kaukos��timen n�pp�inten opettelu",
  },
  { "Phase 1: Detecting RC code type",
    "Phase 1: FB Code feststellen",
    "Faza 1: Sprejemanje IR kode",
    "Fase 1: tipo ricevitore RC",
    "Fase 1: detecteren type afstandsbediening",
    "Fase 1: detectar tipo de receptor",
    "Phase 1: D�tection du type de code",
    "Fase 1: Finne fjernkontroll-kodetype",
    "Vaihe 1: L�hetystavan selvitt�minen",
  },
  { "Press any key on the RC unit",
    "Eine Taste auf der FB dr�cken",
    "Pritisnite tipko na upravljalcu",
    "Premere un tasto nell'unita` RC",
    "Druk op een willekeurige knop",
    "Pressione qualquer tecla do telecomando", 
    "Appuyer sur une touche de la t�l�commande",
    "Trykk en av tastene p� fjernkontrollen",
    "Paina mit� tahansa kaukos��timen n�pp�int�",
  },
  { "RC code detected!",
    "FB Code erkannt!",
    "IR koda sprejeta!",
    "Codice RC rilevato!",
    "Afstandsbediening code herkend!",
    "Codigo do telecomando detectado!",
    "Code de la t�l�commande d�tect�!",
    "Fjernkontroll-kodetype funnet!",
    "N�pp�inpainallus vastaanotettu!",
  },
  { "Do not press any key...",
    "Keine Taste dr�cken...",
    "Ne pritiskajte tipk...",
    "Non premere alcun tasto...",
    "Druk niet op een knop...",
    "Nao pressione nada...",
    "Ne pas appuyer sur une touche ...",
    "Ikke trykk p� noen av tastene...",
    "�l� paina mit��n n�pp�int�...",
  },
  { "Phase 2: Learning specific key codes",
    "Phase 2: Einzelne Tastencodes lernen",
    "Faza 2: Ucenje posebnih kod",
    "Fase 2: Codici specifici dei tasti",
    "Fase 2: Leren specifieke toets-codes",
    "Fase 2: A aprender codigos especificos",
    "Phase 2: Apprentissage des codes des touches",
    "Fase 2: L�re spesifikke tastekoder",
    "Vaihe 2: N�pp�inkoodien opettelu",
  },
  { "Press key for '%s'",
    "Taste f�r '%s' dr�cken",
    "Pritisnite tipko za '%s'",
    "Premere il tasto per '%s'",
    "Druk knop voor '%s'",
    "Pressione tecla para '%s'",
    "Appuyer sur la touche '%s'",
    "Trykk tasten for '%s'",
    "Paina n�pp�int� toiminnolle '%s'",
  },
  { "Press 'Up' to confirm",
    "'Auf' dr�cken zum Best�tigen",
    "Pritisnite tipko 'Gor' za potrditev",
    "Premere 'Su' per confermare",
    "Druk 'Omhoog' om te bevestigen",
    "Pressione 'Cima' para confirmar",
    "Appuyer sur 'Haut' pour confirmer",
    "Trykk 'Opp' for � bekrefte",
    "Paina 'Yl�s' hyv�ksy�ksesi",
  },
  { "Press 'Down' to continue",
    "'Ab' dr�cken zum Weitermachen",
    "Pritisnite tipko 'Dol' za nadaljevanje",
    "Premere 'Giu' per confermare",
    "Druk 'Omlaag' om verder te gaan",
    "Pressione 'Baixo' para continuar",
    "Appuyer sur 'Bas' pour continuer",
    "Trykk Ned' for � fortsette",
    "Paina 'Alas' jatkaaksesi",
  },
  { "(press 'Up' to go back)",
    "('Auf' dr�cken um zur�ckzugehen)",
    "(pritisnite 'Gor' za nazaj)",
    "(premere 'Su' per tornare indietro)",
    "(druk 'Omhoog' om terug te gaan)",
    "(Pressione 'Cima' para voltar)",
    "(Appuyer sur 'Haut' pour revenir en arri�re)",
    "(trykk 'Opp' for � g� tilbake)",
    "(paina 'Yl�s' palataksesi takaisin)",
  },
  { "(press 'Down' to end key definition)",
    "('Ab' dr�cken zum Beenden)",
    "(pritisnite 'Dol' za konec)",
    "('Giu' per finire la definiz tasti)",
    "(Druk 'Omlaag' om te beeindigen)",
    "(Pressione 'Baixo' para terminar a definicao)",
    "(Appuyer sur 'Bas' pour terminer)",
    "(trykk 'Ned' for � avslutte innl�ring)",
    "(paina 'Alas' lopettaaksesi n�pp�inten opettelun)",
  },
  { "Phase 3: Saving key codes",
    "Phase 3: Codes abspeichern",
    "Faza 3: Shranjujem kodo",
    "Fase 3: Salvataggio key codes",
    "Fase 3: Opslaan toets codes",
    "Fase 3: A Salvar os codigos das teclas",
    "Phase 3: Sauvegarde des codes des touches",
    "Fase 3: Lagre tastekoder",
    "Vaihe 3: N�pp�inkoodien tallettaminen",
  },
  { "Press 'Up' to save, 'Down' to cancel",
    "'Auf' speichert, 'Ab' bricht ab",
    "'Gor' za potrditev, 'Dol' za prekinitev",
    "'Su' per salvare, 'Giu' per annullare",
    "'Omhoog' te bewaren, 'Omlaag' voor annuleren",
    "'Cima' para Salvar, 'Baixo' para Cancelar",
    "Appuyer sur 'Haut' pour sauvegarder, 'Bas' pour annuler",
    "Trykk 'Opp' for � lagre, 'Ned' for � avbryte",
    "Paina 'Yl�s' tallettaaksesi ja 'Alas' peruuttaaksesi",
  },
  // Key names:
  { "Up",
    "Auf",
    "Gor",
    "Su",
    "Omhoog",
    "Cima",
    "Haut",
    "Opp",
    "Yl�s",
  },
  { "Down",
    "Ab",
    "Dol",
    "Giu",
    "Omlaag",
    "Baixo",
    "Bas",
    "Ned",
    "Alas",
  },
  { "Menu",
    "Men�",
    "Meni",
    "Menu",
    "Menu",
    "Menu",
    "Menu",
    "Meny",
    "Valikko",
  },
  { "Ok",
    "Ok",
    "Ok",
    "Ok",
    "Ok",
    "Ok",
    "Ok",
    "Ok",
    "Ok",
  },
  { "Back",
    "Zur�ck",
    "Nazaj",
    "Indietro",
    "Terug",
    "Voltar",
    "Retour",
    "Tilbake",
    "Takaisin",
  },
  { "Left",
    "Links",
    "Levo",
    "Sinistra",
    "Links",
    "Esquerda",
    "Gauche",
    "Venstre",
    "Vasemmalle",
  },
  { "Right",
    "Rechts",
    "Desno",
    "Destra",
    "Rechts",
    "Direita",
    "Droite",
    "H�yre",
    "Oikealle",
  },
  { "Red",
    "Rot",
    "Rdeca",
    "Rosso",
    "Rood",
    "Vermelho",
    "Rouge",
    "R�d",
    "Punainen",
  },
  { "Green",
    "Gr�n",
    "Zelena",
    "Verde",
    "Groen",
    "Verde",
    "Vert",
    "Gr�nn",
    "Vihre�",
  },
  { "Yellow",
    "Gelb",
    "Rumena",
    "Giallo",
    "Geel",
    "Amarelo",
    "Jaune",
    "Gul",
    "Keltainen",
  },
  { "Blue",
    "Blau",
    "Modra",
    "Blu",
    "Blauw",
    "Azul",
    "Bleu",
    "Bl�",
    "Sininen",
  },
  { "Power",
    "Ausschalten",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Arr�t",
    "", // TODO
    "Virtakytkin",
  },
  { "Volume+",
    "Lautst�rke+",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Volume+",
    "", // TODO
    "��nenvoimakkuus+",
  },
  { "Volume-",
    "Lautst�rke-",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Volume-",
    "", // TODO
    "��nenvoimakkuus-",
  },
  { "Mute",
    "Stumm",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Coupure du son",
    "", // TODO
    "��nen vaimennus",
  },
  // Miscellaneous:
  { "yes",
    "ja",
    "da",
    "si",
    "ja",
    "sim",
    "oui",
    "ja",
    "kyll�",
  },
  { "no",
    "nein",
    "ne",
    "no",
    "nee",
    "nao",
    "non",
    "nei",
    "ei",
  },
  { "top",
    "oben",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "haut",
    "", // TODO
    "yl�",
  },
  { "bottom",
    "unten",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "bas",
    "", // TODO
    "ala",
  },
  { "free",
    "frei",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "restant",
    "", // TODO
    "vapaa",
  },
  { "Jump: ", // note the trailing blank
    "Springen: ",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Acc�s direct: ",
    "", // TODO
    "Hypp��:",
  },
  { "Volume ", // note the trailing blank
    "Lautst�rke ",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Volume ",
    "", // TODO
    "��nenvoimakkuus ",
  },
  { " Stop replaying", // note the leading blank!
    " Wiedergabe beenden",
    " Prekini ponavljanje",
    " Interrompi riproduzione",
    " Stop afspelen",
    " Parar reproducao",
    " Arr�ter la lecture",
    " Stopp avspilling",
    " Pys�yt� toisto",
  },
  { " Stop recording ", // note the leading and trailing blanks!
    " Aufzeichnung beenden ",
    " Prekini shranjevanje ",
    " Interrompi registrazione ",
    " Stop opnemen ",
    " Parar gravacao ",
    " Arr�ter l'enregistrement ",
    " Stopp opptak fra ",
    " Pys�yt� nauhoitus ",
  },
  { " Cancel editing", // note the leading blank!
    " Schneiden abbrechen",
    " Prekini urejanje",
    " Annulla modifiche",
    " Bewerken afbreken",
    " Anular modificacao",
    " Annuler le montage",
    " Avbryt editering",
    " Peruuta muokkaus",
  },
  { "Switching primary DVB...",
    "Prim�res Interface wird umgeschaltet...",
    "Preklapljanje primarne naprave...",
    "Cambio su card DVB primaria...",
    "Eerste DVB-kaart wordt omgeschakeld...",
    "A mudar placa DVB primaria...",
    "Changement de carte DVB principale...",
    "Bytter hoved DVB-enhet...",
    "Vaihdetaan ensisijainen vastaanotin...",
  },
  { "Up/Dn for new location - OK to move",
    "Auf/Ab f�r neue Position - dann OK",
    "Gor/Dol za novo poz. - Ok za premik",
    "Su/Giu per nuova posizione - OK per muovere",
    "Gebruik Omhoog/Omlaag - daarna Ok",
    "Cima/Baixo para nova localizacao - Ok para mudar",
    "Haut/Bas -> nouvelle place - OK -> d�placer",
    "Opp/Ned for ny plass - OK for � flytte",
    "Yl�s/Alas = liiku, OK = siirr�",
  },
  { "Editing process started",
    "Schnitt gestartet",
    "Urejanje se je zacelo",
    "Processo di modifica iniziato",
    "Bewerken is gestart",
    "Processo de modificacao iniciado",
    "Op�ration de montage lanc�e",
    "Redigeringsprosess startet",
    "Muokkaus aloitettu",
  },
  { "Editing process finished",
    "Schnitt beendet",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Montage termin�",
    "", // TODO
    "Muokkaus lopetettu",
  },
  { "Editing process failed!",
    "Schnitt gescheitert!",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Echec du montage",
    "", // TODO
    "Muokkaus ep�onnistui",
  },
  { "scanning recordings...",
    "Aufzeichnungen werden durchsucht...",
    "", // TODO
    "", // TODO
    "", // TODO
    "", // TODO
    "Recherche des enregistrements...",
    "", // TODO
    "haetaan nauhoituksia...",
  },
  { NULL }
  };

const char *tr(const char *s)
{
  if (Setup.OSDLanguage) {
     for (const tPhrase *p = Phrases; **p; p++) {
         if (strcmp(s, **p) == 0) {
            const char *t = (*p)[Setup.OSDLanguage];
            if (t && *t)
               return t;
            }
         }
     esyslog(LOG_ERR, "no translation found for '%s' in language %d (%s)\n", s, Setup.OSDLanguage, Phrases[0][Setup.OSDLanguage]);
     }
  const char *p = strchr(s, '$');
  return p ? p + 1 : s;
}

const char * const * Languages(void)
{
  return &Phrases[0][0];
}

