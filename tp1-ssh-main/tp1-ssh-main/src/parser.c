#include "parser.h"
#include "tokenizer.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

/**
 * Cette fonction détermine si le token est un argument de commande
 *
 * @param cat Catégorie du token
 * @return 1 si le token est un argument de commande et 0 sinon
 */
int is_args_token(enum token_category cat) {
    // Si le token appartient à une des catégories
    if (cat == TOK_SYMBOL || cat == TOK_STRING_LITERAL)
        return 1;
    return 0;
}

/**
 * Cette fonction convertit la catégorie du token en opérateur de commande
 *
 * @param cat Catégorie du token
 * @return Opérateur correspondant sinon retourne -1 si la catégorie du token n'est pas un opérateur valide
 */
enum op op_token(enum token_category cat) {
    switch (cat) {
        case TOK_PIPE:
            return OP_PIPE;
        case TOK_SEMICOLON:
            return OP_SEPARATOR;
        case TOK_LOGICAL_OR:
            return OP_OR;
        case TOK_LOGICAL_AND:
            return OP_AND;
        case TOK_NEWLINE:
            return OP_TERMINATOR;
        default:
            printf("Invalid token operator type");
            return -1; // Erreur
    }
}


/**
 * Cette fonction prend une liste de tokens et retourne une liste de commandes.
 *
 * @param tokens list chainée de tokens
 * @return list chainée de commandes
 */
struct command* cmd_parse(struct token* tokens)
{
    struct command sentinel = {NULL, NULL, OP_TERMINATOR};
    struct command *current = &sentinel; // Pointeur

    // Pointeur de la liste des tokens
    struct token *token = tokens;

    // Tant qu'il reste des tokens
    while (token != NULL) {

        // Allocation d'une nouvelle commande
        struct command *command = malloc(sizeof(struct command));
        command->next =NULL; // Null puisqu'elle n'est pas encore chainee
        command->op = OP_TERMINATOR; // Par défaut

        int args_counter = 0;
        struct token *tok = token;

        // Tant que les tokens sont des arguments
        while (tok && is_args_token((tok->category))) {
            args_counter++;
            tok = tok->next; // Regarde le prochain
        }

        // Allocation d'un tableau de la taille du compteur d'arguments + 1
        command->args = malloc(sizeof(char*) * (args_counter+1));

        // Copie les valeurs des tokens dans le tableau
        for (int i = 0; i < args_counter; i++) {
            command->args[i] = token->value;
            token = token->next;
        }

        // Termine le tableau avec NULL
        command->args[args_counter] = NULL;

        // Si le token suivant n'est pas un argument, alors c'est un opérateur
        if (token && !is_args_token(token->category)) {
            command->op = op_token(token->category); // Convertit en opérateur
            token = token->next;
        }

        // Met la commande à la liste chainee
        current->next = command;
        current = command;
    }
    return sentinel.next; // Premier élément vrai après le sentinel
}

/**
 * Cette fonction libère la mémoire allouée pour une liste de commandes.
 *
 * @param command list chainée de commandes
 */
void cmd_free(struct command *command)
{
    struct command *current = command;

    // Parcourt chaque élément de la liste
    while (current != NULL) {
        free(current-> args); // Libère la mémoire du tableau args qu'on alloue dans la fonction cmd_parse

        struct command *temp = current; // Garde le pointeur actuel
        current = current-> next; // Avance dans la liste
        free(temp); // Libère la mémoire de la structure
    }
}

/**
 * Cette fonction affiche une liste de commandes.
 * Utilisé pour le débogage.
 *
 * @param commands list chainée de commandes
 */
void cmd_debug_print(const struct command* commands)
{
    for(const struct command* cmd = commands; cmd; cmd = cmd->next)
    {
        for(int i = 0; cmd->args[i]; i++)
        {
            printf("%s ", cmd->args[i]);
        }

        switch (cmd->op) {
            case OP_TERMINATOR: printf("OP_TERMINATOR"); break;
            case OP_SEPARATOR: printf("OP_SEPARATOR"); break;
            case OP_AND: printf("OP_AND"); break;
            case OP_OR: printf("OP_OR"); break;
            case OP_PIPE: printf("OP_PIPE"); break;
            default: printf("OP_INVALID"); break;
        }

        printf("\n");
    }
}

