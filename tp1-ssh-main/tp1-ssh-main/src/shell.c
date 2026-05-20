#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <sys/wait.h>

#include "tokenizer.h"
#include "parser.h"

#define EXECUTION_FAILED (-1)
#define EXECUTION_SUCCESS 1
#define EXECUTION_REQUEST_EXIT 0

/**
 * Cette fonction détermine si l'opérateur peut être skippé ou non
 *
 * @param op Opérateur à analyser
 * @return 1 si on peut sauter l'opérateur et 0 si c'est un opérateur de controle
 */
int is_op_skipped(enum op op) {
    return !(op == OP_OR || op == OP_AND || op == OP_SEPARATOR);
}

/**
 * Cette fonction utilise les descripteurs fournis dans pipe_fd. Cette fonction redirige STDIN et STDOUT selon la position
 * de la commande dans le pipeline
 *
 * @param last_op Opérateur de la commande precedente
 * @param last_pipe_output Descripteur de lecture du pipe precedent
 * @param current_op Opérateur actuel
 * @param pipe_fd Tableau qui contient les deux descripteurs du pipe courant (lecture et ecriture)
 */
void pipe_setup(enum op last_op, int last_pipe_output, enum op current_op, int pipe_fd[2]) {

    // Si la commande precedente utilisait OP_PIPE, alors STDIN de la commande actuelle est redirige du pipe precedent
    if (last_op == OP_PIPE) {
        dup2(last_pipe_output, STDIN_FILENO);
        close(last_pipe_output);
    }

    // Si la commande actuelle utilise OP_PIPE, alors STDOUT est redirigee vers l'extremite ecriture du pipe actuel qui
    // est par la suite fermee dans le processus enfant
    if (current_op == OP_PIPE) {
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);
    }
}

/**
 * Cette fonction execute la commande en prenant en compte l'operateur precedent, le resultat de la commande precedente,
 * et l'état qui indique si on est en cours de saut. La fonction s'occupe aussi de la creation d'un pipe, le fork,
 * l'execution de la commande, la configuration des redirections et retourne le status d'execution
 *
 * @param cmd Commande a executer
 * @param last_op Operateur de la commande precedente
 * @param last_result Resultat de la commande precedente
 * @param in_skip_sequence  Indique si une sequence de commandes est en cours de saut
 * @param last_pipe_output Descripteur de lecture du pipe precedent
 * @return EXECUTION_REQUEST_EXIT, EXECUTION_SUCCESS ou EXECUTION_FAILED
 */
int run_command(const struct command *cmd, enum op last_op, int last_result,
    int *in_skip_sequence, int *last_pipe_output) {

    // Determine si la commande doit etre ignoree a cause de l'operateur precedent
    int should_skip =  (last_op == OP_AND && last_result == EXECUTION_FAILED) ||
        ( last_op == OP_OR && last_result == EXECUTION_SUCCESS); // Vaut 1 si la commande doit etre ignoree ou 0 sinon

    // Si on devrait skipper ou on est deja en train de skip et que l'operateur fait partie de ceux a ignorer
    if (should_skip || (*in_skip_sequence && is_op_skipped(last_op))) {
        *in_skip_sequence = 1; // est en cours de saut
        return last_result;
    }

    *in_skip_sequence = 0; // on n'est plus en cours de saut

    // Si le premier argument est exit, on fait une demande de finir l'execution
    if (strcmp(cmd -> args[0], "exit") == 0)
        return EXECUTION_REQUEST_EXIT;

    // Tableau des deux descripteurs
    int pipe_fd[2] = {-1,-1};

    // créer un pipe que si nécessaire
    if (cmd->op == OP_PIPE) {
        if (pipe(pipe_fd) == -1) {
            perror("pipe");
            exit(EXECUTION_FAILED);
        }
    }

    // Creer un nouveau processus
    pid_t pid = fork();

    // Si la creation du nouveau processus echoue
    if (pid < 0) {
        perror("fork");
        return EXECUTION_FAILED;
    }

    // Processus enfant
    if (pid == 0) {
        // Configuration des redirections STDIN/STDOUT
        pipe_setup(last_op, *last_pipe_output, cmd->op, pipe_fd);

        // Remplace le processus enfant par le programme demande
        // Si execvp reussi, le reste du bloc n'est pas execute
        execvp(cmd->args[0], cmd->args);

        // Si execvp echoue
        fprintf(stderr, "%s: command not found\n", cmd-> args[0]);
        exit(EXECUTION_FAILED);
    }


    int status;


    // Ferme l'extremite d'ecriture du pipe
    if (pipe_fd[1] != -1)
        close(pipe_fd[1]);
    // Stocke l'extremite lecture pour la prochaine commande s'il veut l'utiliser comme entree
    if (pipe_fd[0] != -1)
        *last_pipe_output = pipe_fd[0];
    if (cmd->op != OP_PIPE) {
        // Recupere le code de l'enfant
        waitpid(pid, &status, 0);        
    } else {
        status = 0;
    }

    // Si status est 0, la commande a reussi, sinon elle a echoue
    return (status == 0) ? EXECUTION_SUCCESS : EXECUTION_FAILED;
}


/**
 * Cette fonction prend une liste de commandes et l'exécute.
 *
 * @param cmd list chainée de commandes
 *
 * @return le code de retour de la dernière commande exécutée.
 */
int sh_run(const struct command *cmd)
{
    // Si la liste est vide ou si la premiere commande n'a rien a executer
    if (!cmd || cmd->args[0] == NULL)
        return EXECUTION_FAILED;

    // Puisqu'on n'a pas de commande avant la premiere, rien ne peut empecher la premiere commande de s'executer
    enum op last_op = OP_TERMINATOR;
    int last_result = EXECUTION_SUCCESS;
    int in_skip_sequence = 0;
    int last_pipe_output = STDIN_FILENO;

    // Parcourt la liste de commandes
    for (const struct command *command = cmd; command; command = command-> next) {
        // Execute la commande actuelle et stocke le resultat
        last_result = run_command(command, last_op, last_result, &in_skip_sequence, &last_pipe_output);

        if (last_result == EXECUTION_REQUEST_EXIT)
            return EXECUTION_REQUEST_EXIT;

        // Met a jour l'operateur associe a la commande actuelle comme operateur precedent pour la prochaine commande
        last_op = command->op;
    }

    // Retourne le resultat de la derniere commande executee
    return last_result;
}

int main(void)
{
    for(;;)
    {
        struct token* tokens = tok_next_line();

        if(!tokens) return EXECUTION_FAILED; // Tokenizer error

        //tok_debug_print(tokens);

        struct command* commands = cmd_parse(tokens);

        if(!commands)
        {
            tok_free(tokens);
            return EXECUTION_FAILED; // Parser error
        }

        //cmd_debug_print(commands);

        int status = sh_run(commands);

        if(status == EXECUTION_REQUEST_EXIT)
        {
            exit(0);
        }

        cmd_free(commands);
        tok_free(tokens);
    }
}
