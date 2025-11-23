#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_STATES 20
#define MAX_TRANSITIONS 2000
#define MAX_LEXEME_LENGTH 100
#define MAX_SYMBOL_TABLE_SIZE 97
#define END_SYMBOL -1
typedef enum
{
    IDENTIFIER,
    OPERATEUR,
    NOMBRE,
    DELIMITEUR,
    MOTCLES,
    COMPARATEUR,
    AFECTATION,
    UNKNOWN
} LexemeType;

typedef struct
{
    int row_ptr[MAX_STATES + 1];
    int col_ind[MAX_TRANSITIONS];
    int values[MAX_TRANSITIONS];
} CSRmatrice;

// Structure pour une entrée dans la table des symboles
typedef struct
{
    char lexeme[MAX_LEXEME_LENGTH];
    LexemeType type;
} SymbolEntry;

// Table des symboles
typedef struct
{
    SymbolEntry entries[MAX_SYMBOL_TABLE_SIZE];
    int size;
} TS;

// Structure pour un lexème
typedef struct
{
    char lexeme[MAX_LEXEME_LENGTH];
    LexemeType type;
} Lexeme;
typedef enum
{
    NT_E,      // E
    NT_EPRIME, // E'
    NT_T,      // T
    NT_TPRIME, // T'
    NT_F       // F
} NonTerminal;
typedef enum
{
    TERM_N,           // n (repre sente un nombre ou un identificateur)
    TERM_PLUS,        // +
    TERM_MULT,        // *
    TERM_PAREN_OPEN,  // (
    TERM_PAREN_CLOSE, // )
    TERM_END          // $
} Terminal;
// Definition des productions
typedef enum
{
    PROD_E_TE,           // E -> T E'
    PROD_EPRIME_PLUS_TE, // E' -> + T E'
    PROD_EPRIME_EPSILON, // E' -> ε
    PROD_T_FT,           // T -> F T'
    PROD_TPRIME_MULT_FT, // T' -> * F T'
    PROD_TPRIME_EPSILON, // T' -> ε
    PROD_F_PAREN_E,      // F -> ( E )
    PROD_F_N,            // F -> n
    PROD_ERROR           // Production d'erreur
} Production;

Production parseTable[5][6] = {
    // n      +       *       (       )       $
    {PROD_E_TE, PROD_ERROR, PROD_ERROR, PROD_E_TE, PROD_ERROR, PROD_ERROR},                                       // NT_E
    {PROD_ERROR, PROD_EPRIME_PLUS_TE, PROD_ERROR, PROD_ERROR, PROD_EPRIME_EPSILON, PROD_EPRIME_EPSILON},          // NT_EPRIME
    {PROD_T_FT, PROD_ERROR, PROD_ERROR, PROD_T_FT, PROD_ERROR, PROD_ERROR},                                       // NT_T
    {PROD_ERROR, PROD_TPRIME_EPSILON, PROD_TPRIME_MULT_FT, PROD_ERROR, PROD_TPRIME_EPSILON, PROD_TPRIME_EPSILON}, // NT_TPRIME
    {PROD_F_N, PROD_ERROR, PROD_ERROR, PROD_F_PAREN_E, PROD_ERROR, PROD_ERROR}                                    // NT_F
};

// Structure pour stocker un element de la pile d'analyse
typedef struct
{
    int isTerminal; // 1 si terminal, 0 si non-terminal
    union
    {
        NonTerminal nt;
        Terminal terminal;
    } symbol;
} StackElement;

// Structure pour la pile d'analyse
typedef struct
{
    StackElement elements[100];
    int top;
} ParseStack;

// Fonctions de manipulation de la pile
void initStack(ParseStack *stack)
{
    stack->top = -1;

    // Empiler le symbole de fin ($)
    StackElement endSymbol;
    endSymbol.isTerminal = 1;
    endSymbol.symbol.terminal = TERM_END;
    stack->elements[++stack->top] = endSymbol;

    // Empiler le symbole de depart (E)
    StackElement startSymbol;
    startSymbol.isTerminal = 0;
    startSymbol.symbol.nt = NT_E;
    stack->elements[++stack->top] = startSymbol;
}

void push(ParseStack *stack, StackElement element)
{
    if (stack->top < 99)
    {
        stack->elements[++stack->top] = element;
    }
    else
    {
        printf("Erreur: Débordement de pile\n");
    }
}

StackElement pop(ParseStack *stack)
{
    if (stack->top >= 0)
    {
        return stack->elements[stack->top--];
    }
    else
    {
        printf("Erreur: Pile vide\n");
        StackElement error;
        error.isTerminal = -1;
        return error;
    }
}

StackElement top(ParseStack *stack)
{
    if (stack->top >= 0)
    {
        return stack->elements[stack->top];
    }
    else
    {
        printf("Erreur: Pile vide\n");
        StackElement error;
        error.isTerminal = -1;
        return error;
    }
}

// Conversion LexemeType vers indice de terminal pour la table d'analyse
Terminal convertToTerminal(LexemeType type, const char *lexeme)
{
    // Si c'est la fin de l'entrée
    if (type == UNKNOWN && (strcmp(lexeme, "$") == 0 || lexeme[0] == '\0'))
    {
        return TERM_END;
    }

    switch (type)
    {
    case IDENTIFIER:
    case NOMBRE:
        return TERM_N; // 'n' représente un nombre ou un identificateur dans la grammaire
    case OPERATEUR:
        if (strcmp(lexeme, "+") == 0)
            return TERM_PLUS;
        if (strcmp(lexeme, "*") == 0)
            return TERM_MULT;
        return TERM_END; // Symbole non reconnu
    case DELIMITEUR:
        if (strcmp(lexeme, "(") == 0)
            return TERM_PAREN_OPEN;
        if (strcmp(lexeme, ")") == 0)
            return TERM_PAREN_CLOSE;
        return TERM_END; // Symbole non reconnu
    default:
        return TERM_END; // Symbole non reconnu
    }
}

// Fonction auxiliaire pour afficher les productions
void printProduction(Production prod)
{
    switch (prod)
    {
    case PROD_E_TE:
        printf("Production: E -> T E'\n");
        break;
    case PROD_EPRIME_PLUS_TE:
        printf("Production: E' -> + T E'\n");
        break;
    case PROD_EPRIME_EPSILON:
        printf("Production: E' -> ε\n");
        break;
    case PROD_T_FT:
        printf("Production: T -> F T'\n");
        break;
    case PROD_TPRIME_MULT_FT:
        printf("Production: T' -> * F T'\n");
        break;
    case PROD_TPRIME_EPSILON:
        printf("Production: T' -> ε\n");
        break;
    case PROD_F_PAREN_E:
        printf("Production: F -> ( E )\n");
        break;
    case PROD_F_N:
        printf("Production: F -> n\n");
        break;
    case PROD_ERROR:
        printf("Erreur: Production non définie\n");
        break;
    }
}

// Fonction pour appliquer une production et empiler les symboles correspondants
void applyProduction(ParseStack *stack, Production prod)
{
    StackElement element;

    // Dépiler le non-terminal
    pop(stack);

    // Empiler les éléments de la production en ordre inversé (droite à gauche)
    switch (prod)
    {
    case PROD_E_TE:
        // E -> T E'
        element.isTerminal = 0;
        element.symbol.nt = NT_EPRIME;
        push(stack, element);

        element.isTerminal = 0;
        element.symbol.nt = NT_T;
        push(stack, element);
        break;

    case PROD_EPRIME_PLUS_TE:
        // E' -> + T E'
        element.isTerminal = 0;
        element.symbol.nt = NT_EPRIME;
        push(stack, element);

        element.isTerminal = 0;
        element.symbol.nt = NT_T;
        push(stack, element);

        element.isTerminal = 1;
        element.symbol.terminal = TERM_PLUS;
        push(stack, element);
        break;

    case PROD_EPRIME_EPSILON: // E' -> ε
        break;

    case PROD_T_FT: // T -> F T'
        element.isTerminal = 0;
        element.symbol.nt = NT_TPRIME;
        push(stack, element);

        element.isTerminal = 0;
        element.symbol.nt = NT_F;
        push(stack, element);
        break;

    case PROD_TPRIME_MULT_FT: // T' -> * F T'
        element.isTerminal = 0;
        element.symbol.nt = NT_TPRIME;
        push(stack, element);

        element.isTerminal = 0;
        element.symbol.nt = NT_F;
        push(stack, element);

        element.isTerminal = 1;
        element.symbol.terminal = TERM_MULT;
        push(stack, element);
        break;

    case PROD_TPRIME_EPSILON:
        // T' -> ε
        break;

    case PROD_F_PAREN_E:
        // F -> ( E )
        element.isTerminal = 1;
        element.symbol.terminal = TERM_PAREN_CLOSE;
        push(stack, element);

        element.isTerminal = 0;
        element.symbol.nt = NT_E;
        push(stack, element);

        element.isTerminal = 1;
        element.symbol.terminal = TERM_PAREN_OPEN;
        push(stack, element);
        break;

    case PROD_F_N:
        // F -> n
        element.isTerminal = 1;
        element.symbol.terminal = TERM_N;
        push(stack, element);
        break;

    case PROD_ERROR:
        printf("Erreur: Production non définie\n");
        break;
    }
}

int ajoutSymbole(TS *table, const char *lexeme, LexemeType type);

int hashFonction(const char lexeme[])
{
    int sum = 0;
    for (int i = 0; lexeme[i] != '\0'; i++)
    {
        sum += lexeme[i] * (i + 1);
    }
    return sum % MAX_SYMBOL_TABLE_SIZE;
}
void initialiserTS(TS *table)
{
    table->size = 0;
    for (int i = 0; i < MAX_SYMBOL_TABLE_SIZE; i++)
    {
        table->entries[i].lexeme[0] = '\0';
    }

    ajoutSymbole(table, "if", MOTCLES);
    ajoutSymbole(table, "else", MOTCLES);
    ajoutSymbole(table, "then", MOTCLES);
    ajoutSymbole(table, "while", MOTCLES);
    ajoutSymbole(table, "do", MOTCLES);
    ajoutSymbole(table, "return", MOTCLES);
    ajoutSymbole(table, "fontion", MOTCLES);
    ajoutSymbole(table, "var", MOTCLES);
    ajoutSymbole(table, "const", MOTCLES);
    ajoutSymbole(table, "mod", MOTCLES);
}

int chercherSymbole(TS *table, const char *lexeme)
{
    int cle = hashFonction(lexeme);
    int originaleCle = cle;

    while (1)
    {
        if (strcmp(table->entries[cle].lexeme, "") != 0)
        {
            if (strcmp(table->entries[cle].lexeme, lexeme) == 0)
            {
                return cle;
            }
        }

        cle = (cle + 1) % MAX_SYMBOL_TABLE_SIZE;

        if (cle == originaleCle)
        {
            break;
        }
    }

    return -1;
}

int ajoutSymbole(TS *table, const char *lexeme, LexemeType type)
{
    int index = chercherSymbole(table, lexeme);
    if (index != -1)
    {
        return index;
    }

    int cle = hashFonction((char *)lexeme);
    int cleOriginale = cle;

    while (strcmp(table->entries[cle].lexeme, "") != 0)
    {
        cle = (cle + 1) % MAX_SYMBOL_TABLE_SIZE;
        if (cle == cleOriginale)
        {
            printf("Erreur: Table des symboles pleine\n");
            return -1;
        }
    }

    strncpy(table->entries[cle].lexeme, lexeme, MAX_LEXEME_LENGTH - 1);
    table->entries[cle].lexeme[MAX_LEXEME_LENGTH - 1] = '\0';
    table->entries[cle].type = type;
    table->size++;

    return cle;
}

void afficherTS(TS *table)
{
    printf("\n--- TABLE DES SYMBOLES ---\n");
    for (int i = 0; i < MAX_SYMBOL_TABLE_SIZE; i++)
    {
        if (table->entries[i].lexeme[0] != '\0')
        {
            if (table->entries[i].type == MOTCLES)
                printf("%s mot cle\n", table->entries[i].lexeme);
            else if (table->entries[i].type == IDENTIFIER)
                printf("%s identificateur\n", table->entries[i].lexeme);
            else if (table->entries[i].type == NOMBRE)
                printf("%s num(%s)\n", table->entries[i].lexeme, table->entries[i].lexeme);
        }
    }
    printf("---------------------------\n");
}

void initialiserMatrcie(CSRmatrice *matrice)
{
    memset(matrice, 0, sizeof(CSRmatrice));

    // 0=initial, 1=identifier, 2=OPERATEUR, 3=NOMBRE, 4=eq_op, 5=lt_op, 6=gt_op
    // 7=div_op, 8=comment_start, 9=comment_body, 10=comment_star, 11=délimiteur
    int idx = 0;
    // État 0 (initial) - transitions sortantes
    matrice->row_ptr[0] = 0;
    // Transitions pour les espaces (État 0 -> État 0)
    char whitespace[] = " \t\n\r";
    for (int i = 0; whitespace[i] != '\0'; i++)
    {
        matrice->col_ind[idx] = whitespace[i];
        matrice->values[idx] = 0;
        idx++;
    }
    // Transition pour la division/début de commentaire potentiel
    matrice->col_ind[idx] = '/';
    matrice->values[idx] = 7; // État pour '/' (potentiel début de commentaire)
    idx++;

    // Transitions pour identificateurs (État 0 -> État 1)
    for (char c = 'a'; c <= 'z'; c++)
    {
        matrice->col_ind[idx] = c;
        matrice->values[idx] = 1;
        idx++;
    }

    for (char c = 'A'; c <= 'Z'; c++)
    {
        matrice->col_ind[idx] = c;
        matrice->values[idx] = 1;
        idx++;
    }

    // Transitions pour opérateurs (État 0 -> États spécifiques aux opérateurs)
    // Opérateurs simples (restent dans l'état 2)
    char simple_OPERATEURs[] = "+-*"; // '/' est maintenant traité séparément
    for (int i = 0; simple_OPERATEURs[i] != '\0'; i++)
    {
        matrice->col_ind[idx] = simple_OPERATEURs[i];
        matrice->values[idx] = 2;
        idx++;
    }

    // Délimiteurs
    char delimiteurs[] = ";,(){}";
    for (int i = 0; delimiteurs[i] != '\0'; i++)
    {
        matrice->col_ind[idx] = delimiteurs[i];
        matrice->values[idx] = 11;
        idx++;
    }

    // Opérateurs spéciaux qui peuvent devenir des opérateurs composés
    matrice->col_ind[idx] = '=';
    matrice->values[idx] = 4;
    idx++;

    matrice->col_ind[idx] = '<';
    matrice->values[idx] = 5;
    idx++;

    matrice->col_ind[idx] = '>';
    matrice->values[idx] = 6;
    idx++;

    // Transitions pour NOMBREs (État 0 -> État 3)
    for (char c = '0'; c <= '9'; c++)
    {
        matrice->col_ind[idx] = c;
        matrice->values[idx] = 3;
        idx++;
    }

    matrice->row_ptr[1] = idx;

    // État 1 (identificateur) - transitions sortantes
    // Identificateur peut accepter lettres, chiffres, underscore
    for (char c = 'a'; c <= 'z'; c++)
    {
        matrice->col_ind[idx] = c;
        matrice->values[idx] = 1;
        idx++;
    }

    for (char c = 'A'; c <= 'Z'; c++)
    {
        matrice->col_ind[idx] = c;
        matrice->values[idx] = 1;
        idx++;
    }

    for (char c = '0'; c <= '9'; c++)
    {
        matrice->col_ind[idx] = c;
        matrice->values[idx] = 1;
        idx++;
    }

    matrice->row_ptr[2] = idx;

    // État 2 (opérateur simple) - pas de transitions sortantes
    matrice->row_ptr[3] = idx;

    // État 3 (NOMBRE) - transitions sortantes
    // NOMBRE peut accepter d'autres chiffres
    for (char c = '0'; c <= '9'; c++)
    {
        matrice->col_ind[idx] = c;
        matrice->values[idx] = 3;
        idx++;
    }

    matrice->row_ptr[4] = idx;

    // État 4 (opérateur =) - transition possible vers '=='
    matrice->col_ind[idx] = '=';
    matrice->values[idx] = 13;
    idx++;
    matrice->row_ptr[5] = idx;

    // État 5 (opérateur <) - transition possible vers '<='
    matrice->col_ind[idx] = '=';
    matrice->values[idx] = 13;
    idx++;
    matrice->row_ptr[6] = idx;

    // État 6 (opérateur >) - transition possible vers '>='
    matrice->col_ind[idx] = '=';
    matrice->values[idx] = 13;
    idx++;
    matrice->row_ptr[7] = idx;

    // État 7 (opérateur /) - transitions
    // Si suivi de '*', début de commentaire
    matrice->col_ind[idx] = '*';
    matrice->values[idx] = 8;
    idx++;

    // Pour tous les autres caractères, c'est juste un opérateur de division (ne consomme pas le caractère suivant)
    // => Pas de transition, on reste dans l'état opérateur
    matrice->row_ptr[8] = idx;

    // État 8 (début de commentaire, après '/*') - transitions
    // Tout caractère sauf '*' reste dans le corps du commentaire
    for (int i = 0; i < 128; i++)
    {
        if (i != '*')
        {
            matrice->col_ind[idx] = i;
            matrice->values[idx] = 9;
            idx++;
        }
        else
        {
            matrice->col_ind[idx] = '*';
            matrice->values[idx] = 10;
            idx++;
        }
    }
    matrice->row_ptr[9] = idx;

    // État 9 (corps du commentaire) - transitions
    // Tout caractère sauf '*' reste dans le corps du commentaire
    for (int i = 0; i < 128; i++)
    {
        if (i != '*')
        {
            matrice->col_ind[idx] = i;
            matrice->values[idx] = 9;
            idx++;
        }
        else
        {
            matrice->col_ind[idx] = '*';
            matrice->values[idx] = 10;
            idx++;
        }
    }
    matrice->row_ptr[10] = idx;

    // État 10 (potentielle fin de commentaire, après '*') - transitions
    // Si suivi de '/', fin de commentaire
    matrice->col_ind[idx] = '/';
    matrice->values[idx] = 0;
    idx++;
    // Si suivi de '*', reste dans l'état potentiel fin de commentaire
    matrice->col_ind[idx] = '*';
    matrice->values[idx] = 10;
    idx++;
    // Tout autre caractère retourne au corps du commentaire
    for (int i = 0; i < 128; i++)
    {
        if (i != '*' && i != '/')
        {
            matrice->col_ind[idx] = i;
            matrice->values[idx] = 9;
            idx++;
        }
    }

    matrice->row_ptr[11] = idx;
    matrice->row_ptr[12] = idx;
    matrice->row_ptr[13] = idx;
}

int chercherEtatSuivant(CSRmatrice *matrice, int state, char input)
{
    int start = matrice->row_ptr[state];
    int end = matrice->row_ptr[state + 1];

    for (int i = start; i < end; i++)
    {
        if (matrice->col_ind[i] == input)
        {
            return matrice->values[i];
        }
    }

    return -1;
}

LexemeType getFinaleStatType(int state)
{
    if (state == 1)
        return IDENTIFIER;
    if (state == 2 || state == 7)
        return OPERATEUR;
    if (state == 5 || state == 6 || state == 13)
    {
        return COMPARATEUR;
    }
    if (state == 4)
    {
        return AFECTATION;
    }

    if (state == 3)
        return NOMBRE;
    if (state == 11)
    {
        return DELIMITEUR;
    }

    return UNKNOWN;
}

bool estCommentaire(int state)
{
    return (state == 8 || state == 9 || state == 10);
}

LexemeType lexical_analyzer(CSRmatrice *matrice, const char *input, int *index, TS *table, char *lexeme_buffer)
{
    int Q = 0;
    bool trans = true;
    int lexemeLength = 0;
    bool exitingComment = false;

    // Position de sauvegarde pour implémenter la règle du plus long préfixe
    int savedIndex = *index;
    int savedQ = Q;
    int savedLexemeLength = 0;
    // Implémentation du noyau de l'automate suivant l'algorithme fourni
    char car = input[*index];
    while (trans)
    {
        savedQ = Q;
        Q = chercherEtatSuivant(matrice, Q, car);

        if (Q != -1)
        {

            exitingComment = (estCommentaire(savedQ) && Q == 0);

            if (!(savedQ == 0 && Q == 0))
            {
                lexeme_buffer[lexemeLength++] = car;
            }

            (*index)++;
            LexemeType currentType = getFinaleStatType(Q);
            savedIndex = *index;
            savedQ = Q;
            savedLexemeLength = lexemeLength;

            if (exitingComment)
            {
                lexemeLength = 0;
                savedLexemeLength = 0;
            }
            car = input[*index];
        }
        else
        {
            trans = false;
        }
    }

    *index = savedIndex;
    lexeme_buffer[savedLexemeLength] = '\0';
    LexemeType type = getFinaleStatType(savedQ);
    if (type != UNKNOWN)
    {
        int symbolIndex = chercherSymbole(table, lexeme_buffer);
        if (symbolIndex != -1 && table->entries[symbolIndex].type == MOTCLES)
        {
            type = MOTCLES;
        }
        else if (type == IDENTIFIER)
        {
            ajoutSymbole(table, lexeme_buffer, IDENTIFIER);
        }
        else if (type == NOMBRE)
        {
            ajoutSymbole(table, lexeme_buffer, NOMBRE);
        }

        return type;
    }
    else if (savedLexemeLength > 0)
    {
        printf("Erreur : Lexeme non reconnu - '%s'\n", lexeme_buffer);
        return UNKNOWN;
    }

    return UNKNOWN;
}
void syn_analyzer(CSRmatrice *matrice, const char *input, TS *table)
{
    int index = 0;
    char current_lexeme[MAX_LEXEME_LENGTH];
    LexemeType token_type;
    Terminal current_terminal;
    ParseStack stack;

    printf("\n--- ANALYSE SYNTAXIQUE LL(1) ---\n");

    // 0. Initialiser la pile avec $ et le symbole de départ E
    initStack(&stack);

    // Obtenir le premier symbole (a = in.read())
    token_type = lexical_analyzer(matrice, input, &index, table, current_lexeme);
    current_terminal = convertToTerminal(token_type, current_lexeme);
    printf("Token lu: %s (terminal: %d)\n", current_lexeme, current_terminal);
    while (1)
    {
        // Obtenir le symbole en haut de la pile (x = stack.top())
        StackElement x = top(&stack);

        printf("Sommet de pile: ");
        if (x.isTerminal)
        {
            if (x.symbol.terminal == TERM_END)
                printf("$ (fin)\n");
            else
                printf("Terminal %d\n", x.symbol.terminal);
        }
        else
        {
            printf("Non-terminal NT_%d\n", x.symbol.nt);
        }

        // 1. Si x == $ et a == $, succès
        if (x.isTerminal && x.symbol.terminal == TERM_END && current_terminal == TERM_END)
        {
            printf("Analyse syntaxique réussie!\n");
            return;
        }
        // 2. Si x est un terminal et x == a
        if (x.isTerminal && x.symbol.terminal == current_terminal)
        {
            // Match: depiler x et lire le prochain symbole
            pop(&stack);
            printf("Match: '%s'\n", current_lexeme);

            // a = in.read() - Lire le prochain symbole
            if (input[index] != '\0')
            {
                token_type = lexical_analyzer(matrice, input, &index, table, current_lexeme);
                current_terminal = convertToTerminal(token_type, current_lexeme);
                printf("Token lu: %s (terminal: %d)\n", current_lexeme, current_terminal);
            }
            else
            {
                current_terminal = TERM_END;
                strcpy(current_lexeme, "$"); // la fin
                printf("Fin de l'entrée atteinte \n ");
            }
            continue;
        }

        // 3. Si x est un non-terminal
        else if (!x.isTerminal)
        {
            // Chercher la production dans la table M[x,a]
            Production prod = parseTable[x.symbol.nt][current_terminal];

            if (prod == PROD_ERROR)
            {
                // M[x,a] est une erreur
                printf("Erreur: Pas de production pour le non-terminal %d avec le terminal %d ('%s')\n",
                       x.symbol.nt, current_terminal, current_lexeme);
                break;
            }

            // Appliquer la production
            printf("Application de la production: ");
            printProduction(prod);

            applyProduction(&stack, prod);
            continue;
        }

        printf("Erreur syntaxique: Terminal attendu %d, trouvé %d ('%s')\n",
               x.symbol.terminal, current_terminal, current_lexeme);
        break;
    }

    printf("--- FIN DE L'ANALYSE SYNTAXIQUE AVEC ERREUR ---\n");
}

int main()
{
    CSRmatrice matrice;
    initialiserMatrcie(&matrice);

    TS table;
    initialiserTS(&table);

    const char *input = "10 + abc * (4 * 3) - alpha";
    printf("Analyse de : %s\n", input);

    syn_analyzer(&matrice, input, &table);

    afficherTS(&table);
    return 0;
}