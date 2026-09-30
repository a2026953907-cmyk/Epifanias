// All rights reserved to Kirby64.
//
// VERSAO REFATORADA: struct Jogador / Selecao / Estatisticas / Jogo + enum Tatica.
// Compilar: gcc -std=c99 -Wall -Wextra -o larpball LarpBall_refatorado.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>   // SetConsoleOutputCP, para exibir UTF-8 no console
#endif

#define NUM_IDIOMAS   11
#define NUM_SELECOES  5
#define NUM_JOGADORES 16
#define NUM_TITULARES 11     // indices 0..10 do elenco estao em campo; 11..15 sao reservas
#define MAX_DECISOES  40
#define LIMITE_CANSACO 60    // abaixo disso o jogador e considerado cansado

/* ------------------------------------------------------------------ */
/* Tipos                                                              */
/* ------------------------------------------------------------------ */

enum Idioma {
    PORTUGUES, INGLES, ESPANHOL, ITALIANO, FRANCES, ALEMAO,
    TURCO, ARABE, MANDARIM, JAPONES, COREANO
};

enum Tatica {
    TATICA_OFENSIVA,      // 4-3-3
    TATICA_RETRANCA,      // 5-4-1
    TATICA_EQUILIBRADA,   // 4-4-2
    TATICA_PRESSAO_ALTA,
    TATICA_CONTRA_ATAQUE,
    NUM_TATICAS
};

enum Critico {
    CRITICO_CANSADO,
    CRITICO_FALTA,
    CRITICO_AMARELO,
    CRITICO_LESAO
};

// ANTES: nomesJogadores[16], energiaJogador[16], habilidadeJogador[16], ...
// DEPOIS: um unico tipo que descreve UM jogador.
typedef struct {
    char nome[70];
    int energia;
    int habilidade;
    int velocidade;
    int forca;
    int expulso;
} Jogador;

// ANTES: nomesSelecoes, matrizAtributos, taticasAdversarias e elencos[5][16][70]
// DEPOIS: uma tabela de Selecao, cada uma com seus dados.
typedef struct {
    const char *nome[NUM_IDIOMAS];
    int energia;
    int habilidade;
    int defesa;
    int taticaAdversaria;
    const char *jogadores[NUM_JOGADORES];
} Selecao;

// ANTES: 9 variaveis globais soltas (golsMarcados, golsSofridos, ...)
typedef struct {
    int golsMarcados;
    int golsSofridos;
    int golsPendentesVAR;   // gols ainda nao revisados pelo VAR
    int golsAnulados;
    int entrosamento;
    int substituicoes;
    int ataques;
    int defesas;
    int cartoes;
    char decisoes[MAX_DECISOES][100];
    int totalDecisoes;
} Estatisticas;

// Todo o estado de uma partida em um lugar so: as funcoes recebem Jogo *.
typedef struct {
    const Selecao *minha;
    const Selecao *adversaria;
    Jogador elenco[NUM_JOGADORES];
    int tatica;
    Estatisticas est;
} Jogo;

/* ------------------------------------------------------------------ */
/* Dados                                                              */
/* ------------------------------------------------------------------ */

int idiomaAtual = PORTUGUES;   // unica global que sobrou: texto() depende dela

const char *nomesIdiomas[NUM_IDIOMAS] = {
    "Português", "English", "Español", "Italiano", "Français", "Deutsch",
    "Türkçe", "العربية", "普通话", "日本語", "한국어"
};

static const Selecao selecoes[NUM_SELECOES] = {
    {
        {"Brasil", "Brazil", "Brasil", "Brasile", "Bresil", "Brasilien", "Brezilya", "البرازيل", "巴西", "ブラジル", "브라질"},
        90, 95, 80, TATICA_EQUILIBRADA,
        {
            "Alisson (Liverpool)", "Ederson (Fenerbahce)", "Weverton (Gremio)",
            "Marquinhos (PSG)", "Gabriel Magalhaes (Arsenal)",
            "Casemiro (Manchester United)", "Bruno Guimaraes (Newcastle)",
            "Lucas Paqueta (Flamengo)", "Neymar (Santos)",
            "Raphinha (Barcelona)", "Vini Jr. (Real Madrid)",
            "Bremer (Juventus)", "Danilo (Flamengo)", "Endrick (Lyon)",
            "Gabriel Martinelli (Arsenal)", "Rodrygo (Real Madrid)"
        }
    },
    {
        {"Alemanha", "Germany", "Alemania", "Germania", "Allemagne", "Deutschland", "Almanya", "ألمانيا", "德国", "ドイツ", "독일"},
        85, 80, 95, TATICA_RETRANCA,
        {
            "Oliver Baumann (Hoffenheim)", "Manuel Neuer (Bayern de Munique)",
            "Alexander Nubel (Stuttgart)", "Antonio Rudiger (Real Madrid)",
            "Joshua Kimmich (Bayern de Munique)", "Leon Goretzka (Bayern de Munique)",
            "Jamal Musiala (Bayern de Munique)", "Kai Havertz (Arsenal)",
            "Leroy Sane (Galatasaray)", "Florian Wirtz (Liverpool)",
            "Nick Woltemade (Newcastle)", "Waldemar Anton (Borussia Dortmund)",
            "David Raum (Leipzig)", "Jonathan Tah (Bayern de Munique)",
            "Nadiem Amiri (Mainz)", "Maximilian Beier (Borussia Dortmund)"
        }
    },
    {
        {"Franca", "France", "Francia", "Francia", "France", "Frankreich", "Fransa", "فرنسا", "法国", "フランス", "프랑스"},
        95, 90, 85, TATICA_CONTRA_ATAQUE,
        {
            "Mike Maignan (Milan)", "Robin Risser (Lens)", "Brice Samba (Rennes)",
            "William Saliba (Arsenal)", "Ibrahima Konate (Liverpool)",
            "Aurelien Tchouameni (Real Madrid)", "Adrien Rabiot (Milan)",
            "Kylian Mbappe (Real Madrid)", "Ousmane Dembele (PSG)",
            "Michael Olise (Bayern de Munique)", "Marcus Thuram (Inter de Milao)",
            "Lucas Digne (Aston Villa)", "Theo Hernandez (Al-Hilal)",
            "Jules Kounde (Barcelona)", "Bradley Barcola (PSG)",
            "Rayan Cherki (Manchester City)"
        }
    },
    {
        {"Argentina", "Argentina", "Argentina", "Argentina", "Argentine", "Argentinien", "Arjantin", "الأرجنتين", "阿根廷", "アルゼンチン", "아르헨티나"},
        88, 92, 82, TATICA_OFENSIVA,
        {
            "Emiliano Martinez (Aston Villa)", "Geronimo Rulli (Olympique Marselha)",
            "Juan Musso (Atletico de Madrid)", "Cuti Romero (Tottenham)",
            "Otamendi (Benfica)", "Rodrigo De Paul (Inter Miami)",
            "Enzo Fernandez (Chelsea)", "Mac Allister (Liverpool)",
            "Lionel Messi (Inter Miami)", "Julian Alvarez (Atletico de Madrid)",
            "Lautaro Martinez (Inter de Milao)", "Nahuel Molina (Atletico de Madrid)",
            "Lisandro Martinez (Manchester United)", "Paredes (Boca Juniors)",
            "Thiago Almada (Atletico de Madrid)", "Nico Gonzalez (Atletico de Madrid)"
        }
    },
    {
        {"Italianbrainrots", "Italianbrainrots", "Italianbrainrots", "Italianbrainrots", "Italianbrainrots", "Italianbrainrots", "Italianbrainrots", "إيتاليان براينروتس", "意大利脑洞队", "イタリアンブレインロッツ", "이탈리안 브레인롯츠"},
        85, 90, 75, TATICA_PRESSAO_ALTA,
        {
            "Tung Tung Tung Sahur", "Tralalero Tralala", "Bombardino Crocodilo",
            "Lirili Larila", "Brr Brr Patapim", "Cappuccino Assassino",
            "Ballerina Cappuccina", "La Vaca Saturno Saturnita", "Frigo Camelo",
            "Chimpanzini Bananini", "Boneca Ambalabu",
            "Reserva Italianbrainrot 1", "Reserva Italianbrainrot 2",
            "Reserva Italianbrainrot 3", "Reserva Italianbrainrot 4",
            "Reserva Italianbrainrot 5"
        }
    }
};

/* ------------------------------------------------------------------ */
/* Texto / idiomas                                                    */
/* ------------------------------------------------------------------ */

const char *texto(const char *pt, const char *en, const char *es,
                  const char *it, const char *fr, const char *de,
                  const char *tr, const char *ar, const char *zh,
                  const char *ja, const char *ko) {
    const char *idiomas[] = {pt, en, es, it, fr, de, tr, ar, zh, ja, ko};
    return idiomas[idiomaAtual];   // ponteiro para literal: seguro retornar
}

const char *nomeTatica(int tatica) {
    static const char *taticas[NUM_TATICAS][NUM_IDIOMAS] = {
        {"4-3-3 (Ofensiva)", "4-3-3 (Offensive)", "4-3-3 (Ofensiva)", "4-3-3 (Offensiva)", "4-3-3 (Offensive)", "4-3-3 (Offensiv)", "4-3-3 (Ofansif)", "4-3-3 (هجومية)", "4-3-3（进攻）", "4-3-3（攻撃）", "4-3-3 (공격)"},
        {"5-4-1 (Retranca)", "5-4-1 (Defensive)", "5-4-1 (Defensiva)", "5-4-1 (Difensiva)", "5-4-1 (Defensive)", "5-4-1 (Defensiv)", "5-4-1 (Savunma)", "5-4-1 (دفاعية)", "5-4-1（防守）", "5-4-1（守備）", "5-4-1 (수비)"},
        {"4-4-2 (Equilibrada)", "4-4-2 (Balanced)", "4-4-2 (Equilibrada)", "4-4-2 (Equilibrata)", "4-4-2 (Equilibree)", "4-4-2 (Ausgeglichen)", "4-4-2 (Dengeli)", "4-4-2 (متوازنة)", "4-4-2（均衡）", "4-4-2（バランス）", "4-4-2 (균형)"},
        {"Pressao Alta", "High Press", "Presion Alta", "Pressing Alto", "Pressing Haut", "Hohes Pressing", "Yuksek Pres", "ضغط عال", "高位压迫", "ハイプレス", "높은 압박"},
        {"Contra-ataque", "Counter-attack", "Contraataque", "Contropiede", "Contre-attaque", "Konter", "Kontra atak", "هجمة مرتدة", "反击", "カウンター", "역습"}
    };
    return taticas[tatica][idiomaAtual];
}

const char *nomeSelecao(const Selecao *s) {
    return s->nome[idiomaAtual];
}

/* ------------------------------------------------------------------ */
/* Utilitarios                                                        */
/* ------------------------------------------------------------------ */

static int limitar(int valor, int minimo, int maximo) {
    if (valor < minimo) {
        return minimo;
    }
    if (valor > maximo) {
        return maximo;
    }
    return valor;
}

// Sorteio com chance em %, nunca 0% nem 100% (evita "sempre bloqueia").
static int sucesso(int percentual) {
    return rand() % 100 < limitar(percentual, 2, 98);
}

void limparEntrada(void) {
    int caractere;
    while ((caractere = getchar()) != '\n' && caractere != EOF) {
    }
}

int lerOpcao(int minimo, int maximo) {
    int opcao;
    int lidos;

    while ((lidos = scanf("%d", &opcao)) != 1 || opcao < minimo || opcao > maximo) {
        if (lidos == EOF) {          // correcao: antes o loop nunca terminava em EOF
            printf("\n");
            exit(0);
        }
        printf("%s %d %s %d: ",
               texto("Opcao invalida. Digite um numero entre", "Invalid option. Enter a number between", "Opcion invalida. Introduce un numero entre", "Opzione non valida. Inserisci un numero tra", "Option invalide. Entrez un nombre entre", "Ungultige Option. Geben Sie eine Zahl zwischen", "Gecersiz secenek. Su aralikta bir sayi girin", "خيار غير صالح. أدخل رقماً بين", "选项无效。请输入介于", "無効な選択です。次の範囲の数字を入力：", "잘못된 선택입니다. 다음 범위의 숫자를 입력하세요"),
               minimo,
               texto("e", "and", "y", "e", "et", "und", "ile", "و", "和", "から", "에서"),
               maximo);
        limparEntrada();
    }
    limparEntrada();
    return opcao;
}

void registrarDecisao(Estatisticas *est, const char *descricao) {
    if (est->totalDecisoes < MAX_DECISOES) {
        snprintf(est->decisoes[est->totalDecisoes], sizeof est->decisoes[0], "%s", descricao);
        est->totalDecisoes++;
    }
}

static void ajustarEntrosamento(Jogo *jogo, int delta) {
    jogo->est.entrosamento = limitar(jogo->est.entrosamento + delta, 0, 100);
}

static void cansar(Jogo *jogo, int indice, int quantidade) {
    jogo->elenco[indice].energia = limitar(jogo->elenco[indice].energia - quantidade, 0, 100);
}

/* ------------------------------------------------------------------ */
/* Elenco                                                             */
/* ------------------------------------------------------------------ */

static int emCampo(const Jogo *jogo, int indice) {
    return indice < NUM_TITULARES && !jogo->elenco[indice].expulso;
}

// Sorteia um jogador que esta em campo (para aplicar desgaste, cartao, lesao).
static int sortearEmCampo(const Jogo *jogo) {
    int candidatos[NUM_TITULARES];
    int total = 0;

    for (int i = 0; i < NUM_TITULARES; i++) {
        if (emCampo(jogo, i)) {
            candidatos[total++] = i;
        }
    }
    return total > 0 ? candidatos[rand() % total] : 0;
}

static int mediaEnergiaEmCampo(const Jogo *jogo) {
    int soma = 0;
    int total = 0;

    for (int i = 0; i < NUM_TITULARES; i++) {
        if (emCampo(jogo, i)) {
            soma += jogo->elenco[i].energia;
            total++;
        }
    }
    return total > 0 ? soma / total : 0;
}

// Energia e entrosamento agora influenciam as chances (como o tutorial diz).
static int bonusForma(const Jogo *jogo) {
    return (mediaEnergiaEmCampo(jogo) - 70) / 5
         + (jogo->est.entrosamento - 70) / 10;
}

void inicializarElenco(Jogo *jogo, int indiceSelecao) {
    const Selecao *s = &selecoes[indiceSelecao];

    for (int i = 0; i < NUM_JOGADORES; i++) {
        Jogador *j = &jogo->elenco[i];

        snprintf(j->nome, sizeof j->nome, "%s", s->jogadores[i]);
        j->energia = s->energia - (i % 3) * 3;
        j->habilidade = s->habilidade - (i % 4) * 2;
        j->velocidade = 72 + indiceSelecao * 3 - (i % 4) * 2 + (i % 3) * 3;
        j->forca = 76 + indiceSelecao * 2 - (i % 5) * 2 + (i % 2) * 3;
        j->expulso = 0;
    }
}

void mostrarElencoRecursivo(const Jogo *jogo, int indice, int *energiaTotal) {
    const Jogador *j;

    if (indice == NUM_JOGADORES) {
        return;
    }
    j = &jogo->elenco[indice];

    printf("%d. %-30s | %s: %3d | %s: %3d | %s: %3d | %s: %3d | %s%s\n",
           indice + 1, j->nome,
           texto("Velocidade", "Speed", "Velocidad", "Velocita", "Vitesse", "Geschwindigkeit", "Hiz", "السرعة", "速度", "スピード", "속도"),
           j->velocidade,
           texto("Forca", "Strength", "Fuerza", "Forza", "Force", "Starke", "Guc", "القوة", "力量", "パワー", "힘"),
           j->forca,
           texto("Habilidade", "Skill", "Habilidad", "Abilita", "Technique", "Fertigkeit", "Yetenek", "المهارة", "能力", "スキル", "능력"),
           j->habilidade,
           texto("Energia", "Energy", "Energia", "Energia", "Energie", "Energie", "Enerji", "الطاقة", "体力", "体力", "체력"),
           j->energia,
           indice < NUM_TITULARES ? texto("Titular", "Starter", "Titular", "Titolare", "Titulaire", "Stammspieler", "Ilk 11", "أساسي", "首发", "先発", "선발") :
                                    texto("Reserva", "Substitute", "Suplente", "Riserva", "Remplacant", "Ersatzspieler", "Yedek", "احتياطي", "替补", "控え", "교체 선수"),
           j->expulso ? texto(" | Expulso", " | Sent off", " | Expulsado", " | Espulso", " | Expulse", " | Des Feldes verwiesen", " | Atildi", " | مطرود", " | 被罚下", " | 退場", " | 퇴장") : "");

    *energiaTotal += j->energia;
    mostrarElencoRecursivo(jogo, indice + 1, energiaTotal);
}

void mostrarElenco(const Jogo *jogo) {
    int energiaTotal = 0;

    printf("\n--- %s: %s ---\n",
           texto("ELENCO E TATICAS", "SQUAD AND TACTICS", "PLANTILLA Y TACTICAS", "ROSA E TATTICHE", "EFFECTIF ET TACTIQUES", "KADER UND TAKTIK", "KADRO VE TAKTIKLER", "الفريق والتكتيكات", "阵容和战术", "選手と戦術", "선수단 및 전술"),
           nomeSelecao(jogo->minha));
    printf("%s: %s\n",
           texto("Tatica atual", "Current tactic", "Tactica actual", "Tattica attuale", "Tactique actuelle", "Aktuelle Taktik", "Mevcut taktik", "التكتيك الحالي", "当前战术", "現在の戦術", "현재 전술"),
           nomeTatica(jogo->tatica));

    mostrarElencoRecursivo(jogo, 0, &energiaTotal);

    printf("%s: %d\n",
           texto("Energia media do elenco", "Average squad energy", "Energia media de la plantilla", "Energia media della rosa", "Energie moyenne de l'effectif", "Durchschnittliche Kaderenergie", "Kadro ortalama enerjisi", "متوسط طاقة الفريق", "阵容平均体力", "チーム平均体力", "선수단 평균 체력"),
           energiaTotal / NUM_JOGADORES);
    printf("%s: %s %d | %s %d | %s %d\n\n",
           texto("Atributos", "Attributes", "Atributos", "Attributi", "Attributs", "Attribute", "Ozellikler", "الخصائص", "属性", "能力", "능력"),
           texto("Energia", "Energy", "Energia", "Energia", "Energie", "Energie", "Enerji", "الطاقة", "体力", "体力", "체력"),
           jogo->minha->energia,
           texto("Habilidade", "Skill", "Habilidad", "Abilita", "Technique", "Fertigkeit", "Yetenek", "المهارة", "能力", "スキル", "능력"),
           jogo->minha->habilidade,
           texto("Defesa", "Defense", "Defensa", "Difesa", "Defense", "Verteidigung", "Savunma", "الدفاع", "防守", "ディフェンス", "수비"),
           jogo->minha->defesa);
}

// Correcao: agora recebe QUEM sai (antes trocava sempre o primeiro titular: o goleiro).
void substituirJogador(Jogo *jogo, int indiceSai, int obrigatoria) {
    int indiceEntra = -1;

    for (int i = NUM_TITULARES; i < NUM_JOGADORES; i++) {
        if (!jogo->elenco[i].expulso && jogo->elenco[i].energia > 0) {
            indiceEntra = i;
            break;
        }
    }

    if (indiceEntra == -1) {
        printf("%s.\n", texto("Nao ha jogadores disponiveis para substituicao", "No players are available for substitution", "No hay jugadores disponibles para el cambio", "Non ci sono giocatori disponibili per la sostituzione", "Aucun joueur disponible pour le remplacement", "Keine Spieler fur eine Auswechslung verfugbar", "Oyuncu degisikligi icin uygun oyuncu yok", "لا يوجد لاعب متاح للاستبدال", "没有可替换的球员", "交代できる選手がいません", "교체 가능한 선수가 없습니다"));
        return;
    }

    if (!obrigatoria) {
        printf("%s %s? [1] %s [2] %s: ",
               texto("Deseja substituir", "Replace", "Desea sustituir a", "Vuoi sostituire", "Remplacer", "Auswechseln", "Degistir", "هل تريد استبدال", "是否替换", "交代しますか", "교체하시겠습니까"),
               jogo->elenco[indiceSai].nome,
               texto("Sim", "Yes", "Si", "Si", "Oui", "Ja", "Evet", "نعم", "是", "はい", "예"),
               texto("Nao", "No", "No", "No", "Non", "Nein", "Hayir", "لا", "否", "いいえ", "아니요"));
        if (lerOpcao(1, 2) == 2) {
            cansar(jogo, indiceSai, 10);
            printf("%s %s.\n", jogo->elenco[indiceSai].nome,
                   texto("continua em campo, mas fica cansado", "stays on the field but becomes tired", "sigue en el campo, pero se cansa", "resta in campo, ma si stanca", "reste sur le terrain mais se fatigue", "bleibt auf dem Feld, wird aber mude", "oyunda kalir ama yorulur", "يبقى في الملعب لكنه يتعب", "继续比赛但会疲劳", "出場を続けますが疲れます", "계속 뛰지만 지칩니다"));
            return;
        }
    }

    // Troca as structs inteiras de posicao (atribuicao de struct copia todos os membros).
    Jogador tmp = jogo->elenco[indiceSai];
    jogo->elenco[indiceSai] = jogo->elenco[indiceEntra];
    jogo->elenco[indiceEntra] = tmp;
    jogo->elenco[indiceSai].energia = 85;
    jogo->est.substituicoes++;

    printf("%s: %s %s %s.\n",
           texto("Substituicao", "Substitution", "Cambio", "Sostituzione", "Remplacement", "Auswechslung", "Oyuncu degisikligi", "استبدال", "换人", "交代", "교체"),
           jogo->elenco[indiceSai].nome,
           texto("entra no lugar de", "comes on for", "entra por", "entra al posto di", "remplace", "kommt fur", "yerine giriyor", "يدخل بدلاً من", "替换", "に代わって出場", "대신 들어갑니다"),
           jogo->elenco[indiceEntra].nome);
}

/* ------------------------------------------------------------------ */
/* Taticas                                                            */
/* ------------------------------------------------------------------ */

int bonusAtaque(const Jogo *jogo) {
    switch (jogo->tatica) {
        case TATICA_OFENSIVA:      return 12;
        case TATICA_RETRANCA:      return -10;
        case TATICA_PRESSAO_ALTA:  return 8;
        case TATICA_CONTRA_ATAQUE: return 10;
        default:                   return 0;
    }
}

int bonusDefesa(const Jogo *jogo) {
    switch (jogo->tatica) {
        case TATICA_OFENSIVA:      return -15;
        case TATICA_RETRANCA:      return 15;
        case TATICA_PRESSAO_ALTA:  return 8;
        case TATICA_CONTRA_ATAQUE: return 5;
        default:                   return 0;
    }
}

int desgasteTatica(const Jogo *jogo) {
    switch (jogo->tatica) {
        case TATICA_OFENSIVA:      return 2;
        case TATICA_RETRANCA:      return 0;
        case TATICA_PRESSAO_ALTA:  return 3;
        default:                   return 1;
    }
}

void escolherTatica(Jogo *jogo) {
    printf("\n=============== %s ===============\n", texto("TATICAS", "TACTICS", "TACTICAS", "TATTICHE", "TACTIQUES", "TAKTIKEN", "TAKTIKLER", "التكتيكات", "战术", "戦術", "전술"));
    printf("[1] 4-3-3 (%s)\n", texto("Ofensiva", "Offensive", "Ofensiva", "Offensiva", "Offensive", "Offensiv", "Ofansif", "هجومية", "进攻", "攻撃", "공격"));
    printf("[2] 5-4-1 (%s)\n", texto("Retranca", "Defensive", "Defensiva", "Difensiva", "Defensive", "Defensiv", "Savunma", "دفاعية", "防守", "守備", "수비"));
    printf("[3] 4-4-2 (%s)\n", texto("Equilibrada", "Balanced", "Equilibrada", "Equilibrata", "Equilibree", "Ausgeglichen", "Dengeli", "متوازنة", "均衡", "バランス", "균형"));
    printf("[4] %s\n", texto("Pressao Alta", "High Press", "Presion Alta", "Pressing Alto", "Pressing Haut", "Hohes Pressing", "Yuksek Pres", "ضغط عال", "高位压迫", "ハイプレス", "높은 압박"));
    printf("[5] %s\n", texto("Contra-ataque", "Counter-attack", "Contraataque", "Contropiede", "Contre-attaque", "Konter", "Kontra atak", "هجمة مرتدة", "反击", "カウンター", "역습"));
    printf("%s: ", texto("Escolha sua tatica", "Choose your tactic", "Elige tu tactica", "Scegli la tua tattica", "Choisissez votre tactique", "Wahlen Sie Ihre Taktik", "Taktiginizi secin", "اختر تكتيكك", "选择战术", "戦術を選択", "전술을 선택하세요"));

    jogo->tatica = lerOpcao(1, NUM_TATICAS) - 1;

    printf("%s %s.\n", texto("Tatica alterada para", "Tactic changed to", "Tactica cambiada a", "Tattica cambiata in", "Tactique changee en", "Taktik geandert zu", "Taktik su olarak degistirildi", "تم تغيير التكتيك إلى", "战术已更改为", "戦術を変更しました：", "전술이 다음으로 변경되었습니다:"), nomeTatica(jogo->tatica));
}

/* ------------------------------------------------------------------ */
/* Lances da partida                                                  */
/* ------------------------------------------------------------------ */

void ataque(Jogo *jogo) {
    int jogada;
    int bonus = bonusAtaque(jogo) + bonusForma(jogo);
    int desgaste = desgasteTatica(jogo);
    int habilidade = jogo->minha->habilidade;
    Estatisticas *est = &jogo->est;

    printf("%s\n", texto("Sua selecao recupera a bola! Escolha a jogada:", "Your team wins the ball! Choose the play:", "¡Tu equipo recupera el balon! Elige la jugada:", "La tua squadra recupera la palla! Scegli l'azione:", "Votre equipe recupere le ballon ! Choisissez l'action :", "Ihre Mannschaft gewinnt den Ball! Wahlen Sie den Spielzug:", "Takiminiz topu kazandi! Oyunu secin:", "استعاد فريقك الكرة! اختر الهجمة:", "你的球队夺回了球权！选择进攻方式：", "ボールを奪いました！プレーを選択：", "팀이 공을 되찾았습니다! 플레이를 선택하세요:"));
    printf("[1] %s\n", texto("Tocar a bola (risco baixo)", "Pass the ball (low risk)", "Tocar el balon (riesgo bajo)", "Passare la palla (rischio basso)", "Passer le ballon (faible risque)", "Ball passen (geringes Risiko)", "Pas yap (dusuk risk)", "تمرير الكرة (مخاطرة منخفضة)", "传球（低风险）", "パス（低リスク）", "패스 (낮은 위험)"));
    printf("[2] %s\n", texto("Cruzamento na area (risco medio)", "Cross into the box (medium risk)", "Centro al area (riesgo medio)", "Cross in area (rischio medio)", "Centre dans la surface (risque moyen)", "Flanke in den Strafraum (mittleres Risiko)", "Ceza sahasina orta (orta risk)", "عرضية إلى المنطقة (مخاطرة متوسطة)", "传中（中风险）", "クロス（中リスク）", "크로스 (중간 위험)"));
    printf("[3] %s\n> ", texto("Chute de longe (risco alto)", "Long-range shot (high risk)", "Disparo lejano (riesgo alto)", "Tiro da lontano (rischio alto)", "Tir de loin (risque eleve)", "Fernschuss (hohes Risiko)", "Uzaktan sut (yuksek risk)", "تسديدة بعيدة (مخاطرة عالية)", "远射（高风险）", "ロングシュート（高リスク）", "중거리 슛 (높은 위험)"));

    jogada = lerOpcao(1, 3);
    est->ataques++;

    if (jogada == 1) {
        ajustarEntrosamento(jogo, 2);
        cansar(jogo, sortearEmCampo(jogo), desgaste);
        registrarDecisao(est, texto("Tocou a bola e manteve a posse", "Passed the ball and kept possession", "Toco el balon y mantuvo la posesion", "Ha passato la palla e mantenuto il possesso", "A passe le ballon et garde la possession", "Passte den Ball und behielt den Besitz", "Pas yapti ve topu korudu", "مرر الكرة وحافظ على الاستحواذ", "传球并保持控球", "パスしてボールを保持", "패스하고 점유를 유지"));
        printf("%s\n", texto("A equipe troca passes e mantem a posse.", "The team passes and keeps possession.", "El equipo pasa y mantiene la posesion.", "La squadra passa e mantiene il possesso.", "L'equipe fait circuler le ballon et garde la possession.", "Die Mannschaft passt und behalt den Ball.", "Takim paslasarak topu koruyor.", "يمرر الفريق ويحافظ على الاستحواذ.", "球队传球并保持控球。", "チームはパスを回してボールを保持します。", "팀이 패스하며 점유를 유지합니다."));
    } else if (jogada == 2) {
        ajustarEntrosamento(jogo, 1);
        cansar(jogo, sortearEmCampo(jogo), 3 + desgaste);
        registrarDecisao(est, texto("Tentou cruzamento na area", "Attempted a cross", "Intento un centro al area", "Ha tentato un cross", "A tente un centre", "Versuchte eine Flanke", "Orta denedi", "حاول تمريرة عرضية", "尝试传中", "クロスを試みた", "크로스를 시도"));
        if (sucesso(28 + habilidade / 20 + bonus)) {
            est->golsMarcados++;
            printf("%s\n", texto("GOOOOOOOL! Cruzamento perfeito!", "GOAL! Perfect cross!", "¡GOOOOOL! ¡Centro perfecto!", "GOOOL! Cross perfetto!", "BUUUUT ! Centre parfait !", "TOOOR! Perfekte Flanke!", "GOOOL! Mukemmel orta!", "هدف! عرضية مثالية!", "进球！完美传中！", "ゴール！完璧なクロス！", "골! 완벽한 크로스!"));
        } else {
            printf("%s\n", texto("O cruzamento passa por todo mundo.", "The cross goes past everyone.", "El centro pasa de largo.", "Il cross supera tutti.", "Le centre passe devant tout le monde.", "Die Flanke geht an allen vorbei.", "Orta herkesin onunden geciyor.", "تمر العرضية من أمام الجميع.", "传中球越过了所有人。", "クロスは誰にも合いません。", "크로스가 모두를 지나갑니다."));
        }
    } else {
        ajustarEntrosamento(jogo, -1);
        cansar(jogo, sortearEmCampo(jogo), 5 + desgaste);
        registrarDecisao(est, texto("Arriscou chute de longe", "Took a long-range shot", "Arriesgo un disparo lejano", "Ha tentato un tiro da lontano", "A tente un tir de loin", "Wagte einen Fernschuss", "Uzaktan sut cekti", "خاطر بتسديدة بعيدة", "尝试远射", "ロングシュートを試みた", "중거리 슛을 시도"));
        if (sucesso(18 + habilidade / 10 + bonus)) {
            est->golsMarcados++;
            printf("%s\n", texto("GOOOOOOOL! Um foguete de longe!", "GOAL! A long-range rocket!", "¡GOOOOOL! ¡Un misil desde lejos!", "GOOOL! Un tiro da lontano!", "BUUUUT ! Une frappe de loin !", "TOOOR! Ein Fernschuss!", "GOOOL! Uzaktan roket!", "هدف! صاروخ من بعيد!", "进球！远射轰门！", "ゴール！ロングシュート！", "골! 강력한 중거리 슛!"));
        } else {
            printf("%s\n", texto("O chute sai forte, mas gera contra-ataque.", "The shot is powerful, but it creates a counter-attack.", "El disparo sale fuerte, pero genera un contraataque.", "Il tiro e potente, ma crea un contropiede.", "Le tir est puissant, mais provoque une contre-attaque.", "Der Schuss ist kraftvoll, fuhrt aber zu einem Konter.", "Sut guclu, ancak kontra atak baslatiyor.", "التسديدة قوية لكنها تخلق هجمة مرتدة.", "射门很有力，但给了对手反击机会。", "強烈なシュートですが、カウンターを招きます。", "슛은 강하지만 역습을 허용합니다."));
            if (sucesso(25 - bonus / 2)) {
                est->golsSofridos++;
                printf("%s\n", texto("Contra-ataque perigoso: gol do adversario!", "Dangerous counter-attack: opponent goal!", "Contraataque peligroso: ¡gol del rival!", "Contropiede pericoloso: gol dell'avversario!", "Contre-attaque dangereuse : but adverse !", "Gefahrlicher Konter: Gegentor!", "Tehlikeli kontra atak: rakip golu!", "هجمة مرتدة خطيرة: هدف للخصم!", "危险反击：对手进球！", "危険なカウンター：相手のゴール！", "위험한 역습: 상대 골!"));
            }
        }
    }
}

void defesa(Jogo *jogo) {
    int opcao;
    int bonus = bonusDefesa(jogo) + bonusForma(jogo);
    int defesaBase = jogo->minha->defesa;
    Estatisticas *est = &jogo->est;

    printf("%s\n", texto("O adversario avancou. Escolha a marcacao:", "The opponent advances. Choose the marking:", "El rival avanza. Elige la marca:", "L'avversario avanza. Scegli la marcatura:", "L'adversaire avance. Choisissez le marquage :", "Der Gegner greift an. Wahlen Sie die Deckung:", "Rakip ilerliyor. Markaji secin:", "تقدم الخصم. اختر طريقة المراقبة:", "对手推进了。选择防守方式：", "相手が攻め上がります。マークを選択：", "상대가 전진합니다. 수비 방식을 선택하세요:"));
    printf("[1] %s\n", texto("Marcacao individual (cansa mais)", "Man marking (more tiring)", "Marcaje individual (cansa mas)", "Marcatura a uomo (stanca di piu)", "Marquage individuel (plus fatigant)", "Manndeckung (ermudet mehr)", "Adam adama markaj (daha yorucu)", "مراقبة فردية (أكثر إرهاقاً)", "人盯人（更耗体力）", "マンマーク（疲労大）", "대인 마크 (더 피곤함)"));
    printf("[2] %s\n> ", texto("Marcacao por zona (preserva energia)", "Zone marking (saves energy)", "Marcaje zonal (ahorra energia)", "Marcatura a zona (risparmia energia)", "Marquage de zone (economise l'energie)", "Raumdeckung (spart Energie)", "Alan markaji (enerji tasarrufu)", "مراقبة المنطقة (توفر الطاقة)", "区域防守（节省体力）", "ゾーンマーク（体力節約）", "지역 마크 (체력 절약)"));

    opcao = lerOpcao(1, 2);
    est->defesas++;

    if (opcao == 1) {
        cansar(jogo, sortearEmCampo(jogo), 6 + desgasteTatica(jogo));
        registrarDecisao(est, texto("Usou marcacao individual", "Used man marking", "Uso marcaje individual", "Ha usato la marcatura a uomo", "A utilise le marquage individuel", "Verwendete Manndeckung", "Adam adama markaj kullandi", "استخدم المراقبة الفردية", "采用人盯人防守", "マンマークを使用", "대인 마크 사용"));
        if (sucesso(defesaBase + bonus)) {
            printf("%s\n", texto("A marcacao individual bloqueia o chute!", "Man marking blocks the shot!", "¡El marcaje individual bloquea el disparo!", "La marcatura a uomo blocca il tiro!", "Le marquage individuel bloque le tir !", "Die Manndeckung blockiert den Schuss!", "Adam adama markaj sutu engelliyor!", "المراقبة الفردية تصد التسديدة!", "人盯人防守挡住了射门！", "マンマークでシュートを阻止！", "대인 마크가 슛을 막았습니다!"));
        } else {
            est->golsSofridos++;
            printf("%s\n", texto("O adversario consegue marcar.", "The opponent manages to score.", "El rival consigue marcar.", "L'avversario riesce a segnare.", "L'adversaire parvient a marquer.", "Der Gegner trifft.", "Rakip gol atmayi basardi.", "تمكن الخصم من التسجيل.", "对手成功得分。", "相手が得点しました。", "상대가 득점했습니다."));
        }
    } else {
        cansar(jogo, sortearEmCampo(jogo), 2 + desgasteTatica(jogo) / 2);
        registrarDecisao(est, texto("Usou marcacao por zona", "Used zone marking", "Uso marcaje zonal", "Ha usato la marcatura a zona", "A utilise le marquage de zone", "Verwendete Raumdeckung", "Alan markaji kullandi", "استخدم مراقبة المنطقة", "采用区域防守", "ゾーンマークを使用", "지역 마크 사용"));
        if (sucesso(defesaBase - 18 + bonus)) {
            printf("%s\n", texto("A linha defensiva fecha os espacos.", "The defensive line closes the spaces.", "La linea defensiva cierra los espacios.", "La linea difensiva chiude gli spazi.", "La ligne defensive ferme les espaces.", "Die Abwehr schliesst die Raume.", "Savunma hatti bosluklari kapatiyor.", "الخط الدفاعي يغلق المساحات.", "防线封堵了空间。", "守備ラインがスペースを閉じます。", "수비 라인이 공간을 닫았습니다."));
        } else {
            est->golsSofridos++;
            printf("%s\n", texto("O adversario encontra espaco e marca.", "The opponent finds space and scores.", "El rival encuentra espacio y marca.", "L'avversario trova spazio e segna.", "L'adversaire trouve un espace et marque.", "Der Gegner findet Platz und trifft.", "Rakip bosluk bulup gol atiyor.", "يجد الخصم مساحة ويسجل.", "对手找到空间并得分。", "相手がスペースを見つけて得点。", "상대가 공간을 찾아 득점합니다."));
        }
    }
}

// Revisa UM gol pendente (antes podia anular qualquer gol acumulado).
void verificarVAR(Jogo *jogo) {
    Estatisticas *est = &jogo->est;

    if (est->golsPendentesVAR <= 0) {
        return;
    }
    est->golsPendentesVAR--;

    if (rand() % 100 < 25) {
        printf("%s\n", texto("VAR analisando... GOL ANULADO!", "VAR reviewing... GOAL DISALLOWED!", "El VAR analiza... ¡GOL ANULADO!", "VAR in revisione... GOL ANNULLATO!", "VAR en analyse... BUT ANNULE !", "VAR pruft... TOR ABERKANNT!", "VAR inceliyor... GOL IPTAL!", "الفيديو يراجع... الهدف ملغى!", "VAR审核中……进球无效！", "VAR確認中……ゴール取り消し！", "VAR 판독 중... 골 취소!"));
        est->golsMarcados--;
        est->golsAnulados++;
    } else {
        printf("%s\n", texto("VAR analisando... Gol confirmado!", "VAR reviewing... Goal confirmed!", "El VAR analiza... ¡Gol confirmado!", "VAR in revisione... Gol confermato!", "VAR en analyse... But confirme !", "VAR pruft... Tor bestatigt!", "VAR inceliyor... Gol onaylandi!", "الفيديو يراجع... تم تأكيد الهدف!", "VAR审核中……进球有效！", "VAR確認中……ゴール確定！", "VAR 판독 중... 골 인정!"));
    }
}

// Escolhe os tres melhores cobradores (maior habilidade) entre os que estao em campo.
static void melhoresCobradores(const Jogo *jogo, int cobradores[3]) {
    for (int k = 0; k < 3; k++) {
        int melhor = -1;

        for (int i = 0; i < NUM_TITULARES; i++) {
            int jaEscolhido = 0;

            if (!emCampo(jogo, i)) {
                continue;
            }
            for (int m = 0; m < k; m++) {
                if (cobradores[m] == i) {
                    jaEscolhido = 1;
                }
            }
            if (!jaEscolhido && (melhor == -1 || jogo->elenco[i].habilidade > jogo->elenco[melhor].habilidade)) {
                melhor = i;
            }
        }
        cobradores[k] = melhor;
    }
}

void critico(Jogo *jogo, int tipo) {
    Estatisticas *est = &jogo->est;

    if (tipo == CRITICO_CANSADO) {
        int cansado = -1;

        // Correcao: procura o titular mais cansado de verdade (antes era sempre o jogador 3).
        for (int i = 0; i < NUM_TITULARES; i++) {
            if (emCampo(jogo, i) && (cansado == -1 || jogo->elenco[i].energia < jogo->elenco[cansado].energia)) {
                cansado = i;
            }
        }
        if (cansado == -1 || jogo->elenco[cansado].energia >= LIMITE_CANSACO) {
            return;   // ninguem cansado: o lance nao acontece
        }
        printf("%s: %s (%s: %d)\n",
               texto("Jogador cansado detectado", "Tired player detected", "Jugador cansado detectado", "Giocatore stanco rilevato", "Joueur fatigue detecte", "Muder Spieler erkannt", "Yorgun oyuncu tespit edildi", "تم اكتشاف لاعب متعب", "发现疲劳球员", "疲れた選手を検出", "지친 선수 발견"),
               jogo->elenco[cansado].nome,
               texto("Energia", "Energy", "Energia", "Energia", "Energie", "Energie", "Enerji", "الطاقة", "体力", "体力", "체력"),
               jogo->elenco[cansado].energia);
        substituirJogador(jogo, cansado, 0);
        registrarDecisao(est, texto("Reagiu a jogador cansado", "Responded to a tired player", "Reacciono ante un jugador cansado", "Ha reagito a un giocatore stanco", "A reagi a un joueur fatigue", "Reagierte auf einen muden Spieler", "Yorgun oyuncuya mudahale etti", "استجاب للاعب متعب", "应对疲劳球员", "疲れた選手に対応", "지친 선수에 대응"));
    } else if (tipo == CRITICO_FALTA) {
        int cobradores[3];
        int escolha;

        melhoresCobradores(jogo, cobradores);
        if (cobradores[2] == -1) {
            return;   // menos de 3 jogadores em campo: sem cobranca
        }
        printf("%s\n", texto("Falta perigosa! Quem cobra?", "Dangerous free kick! Who takes it?", "¡Falta peligrosa! ¿Quien la lanza?", "Punizione pericolosa! Chi tira?", "Coup franc dangereux ! Qui tire ?", "Gefahrlicher Freistoss! Wer schiesst?", "Tehlikeli serbest vurus! Kim kullanacak?", "ركلة حرة خطيرة! من سيسدد؟", "危险任意球！谁来主罚？", "危険なフリーキック！誰が蹴りますか？", "위험한 프리킥! 누가 찰까요?"));
        printf("[1] %s [2] %s [3] %s\n> ",
               jogo->elenco[cobradores[0]].nome,
               jogo->elenco[cobradores[1]].nome,
               jogo->elenco[cobradores[2]].nome);
        escolha = lerOpcao(1, 3) - 1;
        registrarDecisao(est, texto("Escolheu cobrador de falta", "Chose the free-kick taker", "Eligio al lanzador de falta", "Ha scelto il tiratore della punizione", "A choisi le tireur du coup franc", "Wahlte den Freistossschutzen", "Serbest vurus kullanicisini secti", "اختار منفذ الركلة الحرة", "选择任意球主罚者", "フリーキックのキッカーを選択", "프리킥 키커 선택"));

        if (sucesso(20 + jogo->elenco[cobradores[escolha]].habilidade / 8)) {
            est->golsMarcados++;
            ajustarEntrosamento(jogo, 3);
            printf("%s\n", texto("GOOOOOOOL DE FALTA!", "FREE-KICK GOAL!", "¡GOOOOOL DE FALTA!", "GOOOL SU PUNIZIONE!", "BUUUUT SUR COUP FRANC !", "FREISTOSSTOR!", "SERBEST VURUS GOLU!", "هدف من ركلة حرة!", "任意球进球！", "フリーキックゴール！", "프리킥 골!"));
        } else {
            printf("%s\n", texto("A bola passa perto, mas nao entra.", "The ball goes close, but misses.", "El balon pasa cerca, pero no entra.", "La palla sfiora, ma non entra.", "Le ballon passe pres, mais ne rentre pas.", "Der Ball geht knapp vorbei.", "Top yakindan geciyor ama girmiyor.", "تمر الكرة بجانب المرمى ولا تدخل.", "球擦边而过但没有进。", "ボールは惜しくも入りません。", "공이 아깝게 빗나갑니다."));
        }
    } else if (tipo == CRITICO_AMARELO) {
        int punido = sortearEmCampo(jogo);

        est->cartoes++;
        printf("%s\n", texto("CARTAO AMARELO!", "YELLOW CARD!", "¡TARJETA AMARILLA!", "CARTELLINO GIALLO!", "CARTON JAUNE !", "GELBE KARTE!", "SARI KART!", "بطاقة صفراء!", "黄牌！", "イエローカード！", "옐로카드!"));
        printf("  -> %s\n", jogo->elenco[punido].nome);
        registrarDecisao(est, texto("Recebeu cartao amarelo", "Received a yellow card", "Recibio una tarjeta amarilla", "Ha ricevuto un cartellino giallo", "A recu un carton jaune", "Erhielt eine gelbe Karte", "Sari kart gordu", "حصل على بطاقة صفراء", "收到黄牌", "イエローカードを受けた", "옐로카드를 받음"));
        if (rand() % 100 < 20) {
            printf("%s\n", texto("Segundo amarelo! Jogador expulso.", "Second yellow! Player sent off.", "¡Segunda amarilla! Jugador expulsado.", "Secondo giallo! Giocatore espulso.", "Deuxieme jaune ! Joueur expulse.", "Zweite Gelbe! Spieler vom Platz gestellt.", "Ikinci sari! Oyuncu atildi.", "البطاقة الصفراء الثانية! طُرد اللاعب.", "第二张黄牌！球员被罚下。", "2枚目のイエロー！退場です。", "두 번째 옐로카드! 선수가 퇴장당합니다."));
            jogo->elenco[punido].expulso = 1;
        }
    } else {   // CRITICO_LESAO
        int lesionado = sortearEmCampo(jogo);

        printf("%s %s.\n", texto("LESAO! Substituicao obrigatoria para", "INJURY! Mandatory substitution for", "¡LESION! Cambio obligatorio para", "INFORTUNIO! Sostituzione obbligatoria per", "BLESSURE ! Remplacement obligatoire pour", "VERLETZUNG! Pflichtwechsel fur", "SAKATLIK! Zorunlu degisiklik:", "إصابة! استبدال إلزامي للاعب", "受伤！必须替换", "負傷！交代が必要です：", "부상! 의무 교체:"), jogo->elenco[lesionado].nome);
        jogo->elenco[lesionado].energia = 0;
        ajustarEntrosamento(jogo, -3);
        substituirJogador(jogo, lesionado, 1);
        registrarDecisao(est, texto("Substituiu jogador lesionado", "Replaced an injured player", "Sustituyo al jugador lesionado", "Ha sostituito il giocatore infortunato", "A remplace le joueur blesse", "Ersetzte einen verletzten Spieler", "Sakatlanan oyuncuyu degistirdi", "استبدل اللاعب المصاب", "替换受伤球员", "負傷した選手を交代", "부상 선수를 교체"));
    }
}

/* ------------------------------------------------------------------ */
/* Partida                                                            */
/* ------------------------------------------------------------------ */

void zerarEstatisticas(Jogo *jogo) {
    memset(&jogo->est, 0, sizeof jogo->est);   // zera a struct inteira de uma vez
    jogo->est.entrosamento = 70;
}

void jogarPartida(Jogo *jogo) {
    int minuto = 0;
    Estatisticas *est;

    zerarEstatisticas(jogo);
    est = &jogo->est;

    printf("\n--- %s: %s x %s ---\n",
           texto("INICIO DA PARTIDA", "MATCH START", "INICIO DEL PARTIDO", "INIZIO DELLA PARTITA", "DEBUT DU MATCH", "SPIELBEGINN", "MAC BASLANGICI", "بداية المباراة", "比赛开始", "試合開始", "경기 시작"),
           nomeSelecao(jogo->minha), nomeSelecao(jogo->adversaria));
    printf("%s: %s\n", texto("Tatica escolhida", "Chosen tactic", "Tactica elegida", "Tattica scelta", "Tactique choisie", "Gewahlte Taktik", "Secilen taktik", "التكتيك المختار", "所选战术", "選択した戦術", "선택한 전술"), nomeTatica(jogo->tatica));

    do {
        int evento;
        int golsAntes;

        minuto += 5;

        printf("\n%d' - %s: %s %d x %d %s | %s: %d\n",
               minuto,
               texto("Placar", "Score", "Marcador", "Risultato", "Score", "Spielstand", "Skor", "النتيجة", "比分", "スコア", "점수"),
               nomeSelecao(jogo->minha), est->golsMarcados,
               est->golsSofridos, nomeSelecao(jogo->adversaria),
               texto("Entrosamento", "Teamwork", "Compañerismo", "Intesa", "Cohesion", "Zusammenspiel", "Uyum", "الانسجام", "默契", "連携", "팀워크"), est->entrosamento);

        golsAntes = est->golsMarcados;
        evento = rand() % 100;

        if (evento < 35) {
            ataque(jogo);
        } else if (evento < 70) {
            defesa(jogo);
        } else if (evento < 80) {
            critico(jogo, CRITICO_CANSADO);
        } else if (evento < 85) {
            critico(jogo, CRITICO_FALTA);
        } else if (evento < 90) {
            critico(jogo, CRITICO_AMARELO);
        } else if (evento < 93) {
            critico(jogo, CRITICO_LESAO);
        } else if (evento < 95) {
            est->golsSofridos++;
            ajustarEntrosamento(jogo, -5);
            printf("%s\n", texto("FRANGO DO GOLEIRO! Gol do adversario!", "GOALKEEPER ERROR! Opponent goal!", "¡ERROR DEL PORTERO! ¡Gol del rival!", "PAPERONE DEL PORTIERE! Gol dell'avversario!", "BOURDE DU GARDIEN ! But adverse !", "TORWARTFEHLER! Gegentor!", "KALECI HATASI! Rakip golu!", "خطأ الحارس! هدف للخصم!", "门将失误！对手进球！", "キーパーのミス！相手のゴール！", "골키퍼 실수! 상대 골!"));
        } else {
            printf("%s\n", texto("O VAR e acionado.", "VAR is called.", "Se llama al VAR.", "Il VAR viene chiamato.", "La VAR est appelee.", "Der VAR wird eingeschaltet.", "VAR cagiriliyor.", "تم استدعاء حكم الفيديو.", "VAR介入。", "VARが呼ばれます。", "VAR이 호출됩니다."));
            if (est->golsPendentesVAR > 0) {
                verificarVAR(jogo);
            } else {
                printf("%s\n", texto("Nao havia gol recente para revisar.", "There was no recent goal to review.", "No habia un gol reciente que revisar.", "Non c'era un gol recente da rivedere.", "Aucun but recent a revoir.", "Es gab kein jungstes Tor zu prufen.", "Incelenecek yakin zamanda gol yok.", "لا يوجد هدف حديث للمراجعة.", "没有最近的进球可供复核。", "確認する最近のゴールはありません。", "검토할 최근 골이 없습니다."));
            }
        }

        // Gol novo neste lance: fica pendente e o VAR pode revisar na hora (30%).
        if (est->golsMarcados > golsAntes) {
            est->golsPendentesVAR += est->golsMarcados - golsAntes;
            if (rand() % 100 < 30) {
                verificarVAR(jogo);
            }
        }
    } while (minuto < 90);

    printf("\n========== %s ==========\n", texto("FIM DE JOGO", "FULL TIME", "FINAL DEL PARTIDO", "FINE DELLA PARTITA", "FIN DU MATCH", "SPIELENDE", "MAC SONU", "نهاية المباراة", "比赛结束", "試合終了", "경기 종료"));
    printf("%s: %s %d x %d %s\n", texto("Placar Final", "Final Score", "Marcador final", "Risultato finale", "Score final", "Endstand", "Son skor", "النتيجة النهائية", "最终比分", "最終スコア", "최종 점수"), nomeSelecao(jogo->minha),
           est->golsMarcados, est->golsSofridos, nomeSelecao(jogo->adversaria));

    printf("%s: %d | %s: %d | %s: %d | %s: %d\n",
           texto("Ataques", "Attacks", "Ataques", "Attacchi", "Attaques", "Angriffe", "Hucumlar", "الهجمات", "进攻", "攻撃", "공격"),
           est->ataques,
           texto("Defesas", "Defenses", "Defensas", "Difese", "Defenses", "Verteidigungen", "Savunmalar", "الدفاعات", "防守", "守備", "수비"),
           est->defesas,
           texto("Substituicoes", "Substitutions", "Sustituciones", "Sostituzioni", "Remplacements", "Auswechslungen", "Oyuncu degisiklikleri", "الاستبدالات", "换人", "交代", "교체"),
           est->substituicoes,
           texto("Cartoes", "Cards", "Tarjetas", "Cartellini", "Cartons", "Karten", "Kartlar", "البطاقات", "黄牌", "カード", "카드"),
           est->cartoes);
    printf("%s: %d\n", texto("Gols anulados pelo VAR", "Goals disallowed by VAR", "Goles anulados por el VAR", "Gol annullati dal VAR", "Buts annules par la VAR", "Vom VAR aberkannte Tore", "VAR tarafindan iptal edilen goller", "الأهداف الملغاة بواسطة الفيديو", "VAR判罚无效的进球", "VARで取り消されたゴール", "VAR 취소 골"), est->golsAnulados);
    printf("%s: %d\n", texto("Entrosamento final", "Final teamwork", "Compañerismo final", "Intesa finale", "Cohesion finale", "Zusammenspiel am Ende", "Mac sonu uyumu", "الانسجام النهائي", "最终默契", "最終連携", "최종 팀워크"), est->entrosamento);
    printf("%s:\n", texto("Resumo das decisoes", "Decision summary", "Resumen de decisiones", "Riepilogo delle decisioni", "Resume des decisions", "Entscheidungszusammenfassung", "Karar ozeti", "ملخص القرارات", "决定摘要", "決定の概要", "결정 요약"));
    for (int i = 0; i < est->totalDecisoes; i++) {
        printf("- %s\n", est->decisoes[i]);
    }

    if (est->golsMarcados > est->golsSofridos) {
        printf("%s\n", texto("VITORIA! O trabalho do tecnico foi decisivo!", "VICTORY! The coach's work was decisive!", "¡VICTORIA! ¡El trabajo del entrenador fue decisivo!", "VITTORIA! Il lavoro dell'allenatore e stato decisivo!", "VICTOIRE ! Le travail de l'entraineur a ete decisif !", "SIEG! Die Arbeit des Trainers war entscheidend!", "ZAFER! Teknik direktorun calismasi belirleyiciydi!", "فوز! كان عمل المدرب حاسماً!", "胜利！教练的工作至关重要！", "勝利！監督の仕事が決定的でした！", "승리! 감독의 활약이 결정적이었습니다!"));
    } else if (est->golsMarcados == est->golsSofridos) {
        printf("%s\n", texto("EMPATE! Uma partida equilibrada.", "DRAW! A balanced match.", "¡EMPATE! Un partido equilibrado.", "PAREGGIO! Una partita equilibrata.", "MATCH NUL ! Un match equilibre.", "UNENTSCHIEDEN! Ein ausgeglichenes Spiel.", "BERABERE! Dengeli bir mac.", "تعادل! مباراة متوازنة.", "平局！势均力敌的比赛。", "引き分け！互角の試合でした。", "무승부! 균형 잡힌 경기였습니다."));
    } else {
        printf("%s\n", texto("DERROTA. A equipe lutou ate o ultimo minuto.", "DEFEAT. The team fought until the final minute.", "DERROTA. El equipo lucho hasta el ultimo minuto.", "SCONFITTA. La squadra ha lottato fino all'ultimo minuto.", "DEFAITE. L'equipe s'est battue jusqu'a la derniere minute.", "NIEDERLAGE. Die Mannschaft kampfte bis zur letzten Minute.", "MAGLUBIYET. Takim son dakikaya kadar mucadele etti.", "هزيمة. قاتل الفريق حتى الدقيقة الأخيرة.", "失败。球队战斗到了最后一分钟。", "敗北。チームは最後まで戦いました。", "패배. 팀은 마지막 순간까지 싸웠습니다."));
    }
}

/* ------------------------------------------------------------------ */
/* Menus                                                              */
/* ------------------------------------------------------------------ */

// ATENCAO: o logo original perdeu os espacos quando eu li o arquivo no GitHub.
// Cole aqui o seu logo() original no lugar deste banner simples.
void logo(void) {
    printf("\n=============================================================================\n");
    printf("                               L A R P B A L L\n");
    printf("=============================================================================\n");
}

void intro(void) {
    const char *mensagens1[NUM_IDIOMAS] = {
        "Voce sera o tecnico durante uma partida decisiva da Copa do Mundo.",
        "You will be the coach during a decisive World Cup match.",
        "Seras el entrenador durante un partido decisivo de la Copa del Mundo.",
        "Sarai l'allenatore durante una partita decisiva della Coppa del Mondo.",
        "Vous serez l'entraineur lors d'un match decisif de la Coupe du Monde.",
        "Sie werden der Trainer in einem entscheidenden Weltmeisterschaftsspiel sein.",
        "Dunya Kupasi'nda belirleyici bir macta teknik direktor olacaksiniz.",
        "ستكون المدرب خلال مباراة حاسمة في كأس العالم.",
        "你将在一场关键的世界杯比赛中担任教练。",
        "あなたはワールドカップの重要な試合で監督を務めます。",
        "당신은 월드컵의 중요한 경기에서 감독을 맡게 됩니다."
    };
    const char *mensagens2[NUM_IDIOMAS] = {
        "Tome decisoes, administre a energia do elenco e tente vencer em 90 minutos.",
        "Make decisions, manage the squad's energy and try to win in 90 minutes.",
        "Toma decisiones, administra la energia del equipo e intenta ganar en 90 minutos.",
        "Prendi decisioni, gestisci l'energia della squadra e prova a vincere in 90 minuti.",
        "Prenez des decisions, gerez l'energie de l'equipe et essayez de gagner en 90 minutes.",
        "Treffen Sie Entscheidungen, verwalten Sie die Energie des Kaders und versuchen Sie, in 90 Minuten zu gewinnen.",
        "Kararlar alin, takimin enerjisini yonetin ve 90 dakikada kazanmaya calisin.",
        "اتخذ القرارات وأدر طاقة الفريق وحاول الفوز خلال 90 دقيقة.",
        "做出决定，管理球队体力，并努力在90分钟内获胜。",
        "決断を下し、チームの体力を管理して、90分で勝利を目指します。",
        "결정을 내리고 팀의 체력을 관리하며 90분 안에 승리하세요."
    };

    printf("%s\n", mensagens1[idiomaAtual]);
    printf("%s\n\n", mensagens2[idiomaAtual]);
}

void escolherIdioma(void) {
    int opcao;

    printf("\n=========== %s ===========\n",
           texto("SELECAO DE IDIOMA", "LANGUAGE SELECTION", "SELECCION DE IDIOMA",
                 "SELEZIONE DELLA LINGUA", "SELECTION DE LA LANGUE",
                 "SPRACHAUSWAHL", "DIL SECIMI", "اختيار اللغة", "语言选择",
                 "言語選択", "언어 선택"));

    for (int i = 0; i < NUM_IDIOMAS; i++) {
        printf("[%d] %s\n", i + 1, nomesIdiomas[i]);
    }
    printf("> ");
    opcao = lerOpcao(1, NUM_IDIOMAS);
    idiomaAtual = opcao - 1;

    printf("%s: %s\n", texto("Idioma selecionado", "Selected language",
                             "Idioma seleccionado", "Lingua selezionata",
                             "Langue selectionnee", "Ausgewahlte Sprache",
                             "Secilen dil", "اللغة المختارة", "已选择语言",
                             "選択した言語", "선택한 언어"),
           nomesIdiomas[idiomaAtual]);
}

void mostrarTutorial(void) {
    printf("\n================ %s ================\n",
           texto("TUTORIAL", "TUTORIAL", "TUTORIAL", "TUTORIAL", "TUTORIEL",
                 "TUTORIAL", "REHBER", "دليل", "教程", "チュートリアル", "튜토리얼"));
    printf("%s\n", texto("ENERGIA: preparo fisico. Diminui durante as jogadas e marcacoes.",
                         "ENERGY: physical fitness. It decreases during plays and marking.",
                         "ENERGIA: preparacion fisica. Disminuye durante las jugadas y marcajes.",
                         "ENERGIA: preparazione fisica. Diminuisce durante le azioni e le marcature.",
                         "ENERGIE : condition physique. Elle diminue pendant les actions et le marquage.",
                         "ENERGIE: Fitness. Sie sinkt wahrend Spielzugen und Deckung.",
                         "ENERJI: fiziksel hazirlik. Oyunlar ve markaj sirasinda azalir.",
                         "الطاقة: اللياقة البدنية. تنخفض أثناء اللعب والمراقبة.",
                         "体力：身体状态，会在比赛和盯防中下降。",
                         "体力：プレーやマークで減少します。",
                         "체력: 신체 상태이며 플레이와 마크 중 감소합니다."));
    printf("%s\n", texto("HABILIDADE: qualidade tecnica. Aumenta a chance de jogadas resultarem em gol.",
                         "SKILL: technical quality. It increases the chance of plays resulting in goals.",
                         "HABILIDAD: calidad tecnica. Aumenta la posibilidad de que las jugadas terminen en gol.",
                         "ABILITA: qualita tecnica. Aumenta la probabilita che le azioni producano gol.",
                         "TECHNIQUE : qualite technique. Elle augmente les chances de marquer.",
                         "FERTIGKEIT: technische Qualitat. Sie erhoht die Chance auf Tore.",
                         "YETENEK: teknik kalite. Oyunlarin gole donusme sansini artirir.",
                         "المهارة: الجودة الفنية. تزيد فرصة تسجيل الأهداف.",
                         "能力：技术水平，会提高进攻得分的机会。",
                         "スキル：技術力。プレーがゴールになる確率を高めます。",
                         "능력: 기술적 수준이며 득점 가능성을 높입니다."));
    printf("%s\n", texto("DEFESA: capacidade de impedir gols do adversario.",
                         "DEFENSE: ability to prevent the opponent from scoring.",
                         "DEFENSA: capacidad de impedir los goles del rival.",
                         "DIFESA: capacita di impedire i gol dell'avversario.",
                         "DEFENSE : capacite d'empecher l'adversaire de marquer.",
                         "VERTEIDIGUNG: Fahigkeit, Gegentore zu verhindern.",
                         "SAVUNMA: rakibin gol atmasini engelleme yetenegi.",
                         "الدفاع: القدرة على منع الخصم من التسجيل.",
                         "防守：阻止对手进球的能力。",
                         "ディフェンス：相手の得点を防ぐ能力。",
                         "수비: 상대의 득점을 막는 능력."));
    printf("%s\n", texto("ENTROSAMENTO: cooperacao da equipe. Passes aumentam; erros e lesoes diminuem.",
                         "TEAMWORK: team cooperation. Passes increase it; mistakes and injuries reduce it.",
                         "COMPAÑERISMO: cooperacion del equipo. Los pases lo aumentan; errores y lesiones lo reducen.",
                         "INTESA: cooperazione della squadra. I passaggi la aumentano; errori e infortuni la riducono.",
                         "COHESION : cooperation de l'equipe. Les passes l'augmentent ; les erreurs et blessures la reduisent.",
                         "ZUSAMMENSPIEL: Zusammenarbeit. Passe erhohen es; Fehler und Verletzungen senken es.",
                         "UYUM: takim isbirligi. Paslar artirir; hatalar ve sakatliklar azaltir.",
                         "الانسجام: تعاون الفريق. التمريرات تزيده والأخطاء والإصابات تقلله.",
                         "默契：团队合作。传球会增加，失误和受伤会降低。",
                         "連携：チームの協力。パスで上がり、ミスや負傷で下がります。",
                         "팀워크: 팀의 협력. 패스로 증가하고 실수와 부상으로 감소합니다."));
    printf("%s\n", texto("ATAQUE: tocar e seguro; cruzar equilibra risco; chutar e mais arriscado.",
                         "ATTACK: passing is safe; crossing balances risk; shooting is riskier.",
                         "ATAQUE: pasar es seguro; centrar equilibra el riesgo; chutar es mas arriesgado.",
                         "ATTACCO: passare e sicuro; il cross bilancia il rischio; tirare e piu rischioso.",
                         "ATTAQUE : faire une passe est sur ; centrer equilibre le risque ; tirer est plus risque.",
                         "ANGRIFF: Passen ist sicher; Flanken gleichen das Risiko aus; Schiessen ist riskanter.",
                         "HUCUM: paslasmak guvenlidir; orta risklidir; sut daha risklidir.",
                         "الهجوم: التمرير آمن، العرضية متوازنة المخاطر، والتسديد أكثر خطورة.",
                         "进攻：传球安全，传中风险适中，远射风险更高。",
                         "攻撃：パスは安全、クロスは中程度、シュートは高リスクです。",
                         "공격: 패스는 안전하고 크로스는 보통 위험, 슛은 더 위험합니다."));
    printf("===========================================\n");
}

int escolherSelecao(void) {
    int opcao;

    printf("%s:\n", texto("Escolha sua selecao", "Choose your team", "Elige tu seleccion", "Scegli la tua nazionale", "Choisissez votre equipe", "Wahlen Sie Ihre Mannschaft", "Takiminizi secin", "اختر منتخبك", "选择你的球队", "代表チームを選択", "국가대표팀을 선택하세요"));
    for (int i = 0; i < NUM_SELECOES; i++) {
        printf("[%d] %s (%s %d | %s %d | %s %d)\n",
               i + 1, nomeSelecao(&selecoes[i]),
               texto("Energia", "Energy", "Energia", "Energia", "Energie", "Energie", "Enerji", "الطاقة", "体力", "体力", "체력"),
               selecoes[i].energia,
               texto("Habilidade", "Skill", "Habilidad", "Abilita", "Technique", "Fertigkeit", "Yetenek", "المهارة", "能力", "スキル", "능력"),
               selecoes[i].habilidade,
               texto("Defesa", "Defense", "Defensa", "Difesa", "Defense", "Verteidigung", "Savunma", "الدفاع", "防守", "ディフェンス", "수비"),
               selecoes[i].defesa);
    }
    printf("> ");
    opcao = lerOpcao(1, NUM_SELECOES);
    printf("%s %s!\n", texto("Voce escolheu", "You chose", "Has elegido", "Hai scelto", "Vous avez choisi", "Sie haben gewahlt", "Sectiniz", "لقد اخترت", "你选择了", "選択したチーム：", "선택한 팀:"), nomeSelecao(&selecoes[opcao - 1]));
    return opcao - 1;
}

int sortearAdversario(int selecaoEscolhida) {
    int adversario;

    do {
        adversario = rand() % NUM_SELECOES;
    } while (adversario == selecaoEscolhida);
    return adversario;
}

void mostrarAdversario(const Jogo *jogo) {
    const Selecao *adv = jogo->adversaria;

    printf("\n=========== %s ===========\n", texto("ANALISE DO ADVERSARIO", "OPPONENT ANALYSIS", "ANALISIS DEL RIVAL", "ANALISI DELL'AVVERSARIO", "ANALYSE DE L'ADVERSAIRE", "GEGNERANALYSE", "RAKIP ANALIZI", "تحليل الخصم", "对手分析", "相手分析", "상대 분석"));
    printf("%s: %s\n", texto("Proximo adversario", "Next opponent", "Proximo rival", "Prossimo avversario", "Prochain adversaire", "Nachster Gegner", "Siradaki rakip", "الخصم القادم", "下一个对手", "次の相手", "다음 상대"), nomeSelecao(adv));
    printf("%s: %s\n", texto("Tatica adversaria", "Opponent tactic", "Tactica rival", "Tattica avversaria", "Tactique adverse", "Gegnerische Taktik", "Rakip taktigi", "تكتيك الخصم", "对手战术", "相手の戦術", "상대 전술"), nomeTatica(adv->taticaAdversaria));
    printf("%s: %s %d | %s %d | %s %d\n",
           texto("Atributos", "Attributes", "Atributos", "Attributi", "Attributs", "Attribute", "Ozellikler", "الخصائص", "属性", "能力", "능력"),
           texto("Energia", "Energy", "Energia", "Energia", "Energie", "Energie", "Enerji", "الطاقة", "体力", "体力", "체력"),
           adv->energia,
           texto("Habilidade", "Skill", "Habilidad", "Abilita", "Technique", "Fertigkeit", "Yetenek", "المهارة", "能力", "スキル", "능력"),
           adv->habilidade,
           texto("Defesa", "Defense", "Defensa", "Difesa", "Defense", "Verteidigung", "Savunma", "الدفاع", "防守", "ディフェンス", "수비"),
           adv->defesa);
    printf("=============================================\n");
}

void prepararPartida(Jogo *jogo) {
    int opcao;

    do {
        printf("\n=============== %s ===============\n", texto("PRE-JOGO", "PRE-MATCH", "PREPARTIDO", "PRE-PARTITA", "AVANT-MATCH", "VORSPIEL", "MAC ONCESI", "ما قبل المباراة", "赛前", "試合前", "경기 전"));
        printf("%s x %s\n", nomeSelecao(jogo->minha), nomeSelecao(jogo->adversaria));
        printf("%s: %s\n", texto("Sua tatica", "Your tactic", "Tu tactica", "La tua tattica", "Votre tactique", "Ihre Taktik", "Taktiginiz", "تكتيكك", "你的战术", "あなたの戦術", "내 전술"), nomeTatica(jogo->tatica));
        mostrarAdversario(jogo);

        printf("[1] %s\n", texto("Confirmar e iniciar partida", "Confirm and start match", "Confirmar e iniciar partido", "Conferma e inizia la partita", "Confirmer et commencer le match", "Bestatigen und Spiel starten", "Onayla ve maci baslat", "تأكيد وبدء المباراة", "确认并开始比赛", "確認して試合開始", "확인하고 경기 시작"));
        printf("[2] %s\n", texto("Alterar sua tatica", "Change your tactic", "Cambiar tu tactica", "Cambia la tua tattica", "Changer votre tactique", "Taktik andern", "Taktiginizi degistirin", "تغيير تكتيكك", "更改你的战术", "戦術を変更", "전술 변경"));
        printf("[3] %s\n> ", texto("Cancelar e voltar ao menu", "Cancel and return to menu", "Cancelar y volver al menu", "Annulla e torna al menu", "Annuler et revenir au menu", "Abbrechen und zum Menu", "Iptal et ve menuye don", "إلغاء والعودة إلى القائمة", "取消并返回菜单", "キャンセルしてメニューへ", "취소하고 메뉴로 돌아가기"));

        opcao = lerOpcao(1, 3);

        if (opcao == 2) {
            escolherTatica(jogo);
        } else if (opcao == 1) {
            jogarPartida(jogo);
        }
    } while (opcao == 2);
}

int main(void) {
    char tecnico[60];
    int indiceSelecao;
    int indiceAdversaria;
    int opcao;
    Jogo jogo;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    srand((unsigned)time(NULL));

    logo();
    escolherIdioma();
    intro();

    printf("%s: ", texto("Digite o nome do tecnico", "Enter the coach's name", "Escribe el nombre del entrenador", "Inserisci il nome dell'allenatore", "Entrez le nom de l'entraineur", "Geben Sie den Trainernamen ein", "Teknik direktorun adini girin", "أدخل اسم المدرب", "输入教练姓名", "監督の名前を入力", "감독 이름을 입력하세요"));
    if (fgets(tecnico, sizeof tecnico, stdin) == NULL) {
        snprintf(tecnico, sizeof tecnico, "Tecnico");
    } else {
        tecnico[strcspn(tecnico, "\n")] = '\0';
        if (tecnico[0] == '\0') {
            snprintf(tecnico, sizeof tecnico, "Tecnico");
        }
    }

    indiceSelecao = escolherSelecao();
    indiceAdversaria = sortearAdversario(indiceSelecao);

    memset(&jogo, 0, sizeof jogo);
    jogo.minha = &selecoes[indiceSelecao];
    jogo.adversaria = &selecoes[indiceAdversaria];
    jogo.tatica = TATICA_EQUILIBRADA;
    inicializarElenco(&jogo, indiceSelecao);

    do {
        printf("\n=========== %s ===========\n", texto("MENU PRINCIPAL", "MAIN MENU", "MENU PRINCIPAL", "MENU PRINCIPALE", "MENU PRINCIPAL", "HAUPTMENÜ", "ANA MENU", "القائمة الرئيسية", "主菜单", "メインメニュー", "메인 메뉴"));
        printf("%s: %s | %s: %s\n",
               texto("Tecnico", "Coach", "Entrenador", "Allenatore", "Entraineur", "Trainer", "Teknik direktor", "المدرب", "教练", "監督", "감독"),
               tecnico,
               texto("Selecao", "Team", "Seleccion", "Nazionale", "Equipe", "Mannschaft", "Takim", "المنتخب", "球队", "代表チーム", "팀"),
               nomeSelecao(jogo.minha));
        printf("%s: %s\n", texto("Tatica atual", "Current tactic", "Tactica actual", "Tattica attuale", "Tactique actuelle", "Aktuelle Taktik", "Mevcut taktik", "التكتيك الحالي", "当前战术", "現在の戦術", "현재 전술"), nomeTatica(jogo.tatica));
        printf("%s: %s | %s: %s\n",
               texto("Proximo adversario", "Next opponent", "Proximo rival", "Prossimo avversario", "Prochain adversaire", "Nachster Gegner", "Siradaki rakip", "الخصم القادم", "下一个对手", "次の相手", "다음 상대"),
               nomeSelecao(jogo.adversaria),
               texto("Tatica", "Tactic", "Tactica", "Tattica", "Tactique", "Taktik", "Taktik", "التكتيك", "战术", "戦術", "전술"),
               nomeTatica(jogo.adversaria->taticaAdversaria));

        printf("[1] %s\n[2] %s\n[3] %s\n",
               texto("Ver pre-jogo e iniciar partida", "View pre-match and start", "Ver prepartido e iniciar", "Vedi pre-partita e inizia", "Voir l'avant-match et commencer", "Vorspiel ansehen und starten", "Mac oncesini gor ve baslat", "عرض ما قبل المباراة وابدأ", "查看赛前并开始", "試合前画面と開始", "경기 전 보기 및 시작"),
               texto("Ver elenco e taticas", "View squad and tactics", "Ver plantilla y tacticas", "Vedi rosa e tattiche", "Voir l'effectif et les tactiques", "Kader und Taktik ansehen", "Kadro ve taktigi gor", "عرض الفريق والتكتيكات", "查看阵容和战术", "選手と戦術を見る", "선수단 및 전술 보기"),
               texto("Alterar tatica", "Change tactic", "Cambiar tactica", "Cambia tattica", "Changer de tactique", "Taktik andern", "Taktigi degistir", "تغيير التكتيك", "更改战术", "戦術を変更", "전술 변경"));
        printf("[4] %s\n[5] %s\n[6] %s\n> ",
               texto("Ver adversario", "View opponent", "Ver rival", "Vedi avversario", "Voir l'adversaire", "Gegner ansehen", "Rakibi gor", "عرض الخصم", "查看对手", "相手を見る", "상대 보기"),
               texto("Ver tutorial", "View tutorial", "Ver tutorial", "Vedi tutorial", "Voir le tutoriel", "Tutorial ansehen", "Rehberi gor", "عرض الدليل", "查看教程", "チュートリアルを見る", "튜토리얼 보기"),
               texto("Sair", "Exit", "Salir", "Esci", "Quitter", "Beenden", "Cikis", "خروج", "退出", "終了", "종료"));

        opcao = lerOpcao(1, 6);

        if (opcao == 1) {
            prepararPartida(&jogo);
        } else if (opcao == 2) {
            mostrarElenco(&jogo);
        } else if (opcao == 3) {
            escolherTatica(&jogo);
        } else if (opcao == 4) {
            mostrarAdversario(&jogo);
        } else if (opcao == 5) {
            mostrarTutorial();
        } else {
            printf("%s %s!\n", texto("Ate a proxima, tecnico", "See you next time, coach", "Hasta pronto, entrenador", "A presto, allenatore", "A bientot, entraineur", "Bis bald, Trainer", "Gorusuruz, teknik direktor", "إلى اللقاء أيها المدرب", "下次见，教练", "また会いましょう、監督", "다음에 뵙겠습니다, 감독님"), tecnico);
        }
    } while (opcao != 6);

    return 0;
}
// Este código foi aprimorado com IA para correção de bugs, a versão original segue em um arquivo separado
