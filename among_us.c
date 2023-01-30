#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

// the dimensions of the grid
#define GRID_DIM 2*N + 1

// number of astronauts and impostors
int astro_count, imp_count;

// number of dead astronauts and impostors
int dead_astro_count, dead_imp_count;

// the dimension parameter N
int N;

// the number of iterations to simulate
int iteration;

// the status of the game
enum game_status {
    Defeat, // all astronauts are dead
    Victory, // all impostors are dead
    Continue // the game is still ongoing
};

// the current game status
enum game_status g_stat;

// a structure representing an astronaut
struct Astronaut {
    bool is_alive; // whether the astronaut is alive or not
    int x, y; // the current coordinates of the astronaut in the grid
};

// a structure representing an impostor
struct Impostor {
    bool is_alive; // whether the impostor is alive or not
    int x, y; // the current coordinates of the impostor in the grid
};

// a structure representing the people in the game (astronauts and impostors)
struct People {
    struct Astronaut** astronauts; // the array of astronauts
    struct Impostor** impostors; // the array of imposters
};


// prints the current state of the game in ASCII characters
void print_state(struct People* people) {
    // create the grid with dimensions 2 * N + 1
    char grid[GRID_DIM][GRID_DIM];
    for (int i = 0; i < GRID_DIM; i++) {
        for (int j = 0; j < GRID_DIM; j++) {
            grid[i][j] = 'O';
        }
    }

    // place the astronauts and impostors in the grid
    for (int i = 0; i < astro_count; i++) {
        if (people->astronauts[i]->is_alive) {
            grid[people->astronauts[i]->y][people->astronauts[i]->x] = 'A';
        }
    }
    for (int i = 0; i < imp_count; i++) {
        if (people->impostors[i]->is_alive) {
            grid[people->impostors[i]->y][people->impostors[i]->x] = 'I';
        }
    }

    // print the grid with a bar between each cell and for the walls
    for (int i = 0; i < GRID_DIM; i++) {
        for (int j = 0; j < GRID_DIM; j++) {
            printf("|%c", grid[i][j]);
        }
       	printf("|\n");
    }
    printf("\n");
}


        



// moves the astronauts in the game
void move_astronauts(struct Astronaut** astronauts) {
    for (int i = 0; i < astro_count; i++) {
        // if the astronaut is dead, we don't need to move it
        bool moved_y = false;
        if (!astronauts[i]->is_alive) {
            continue;
        }

        // first, the astronaut moves vertically to reach the middle row
        if (astronauts[i]->y < N) {
            astronauts[i]->y++;
            moved_y = true;
        }
        else if (astronauts[i]->y > N) {
            astronauts[i]->y--;
            moved_y = true;
        }
        // then, the astronaut moves horizontally to reach the middle column
        if ((astronauts[i]->y == N) && (!moved_y)) {
            if (astronauts[i]->x < N) {
                astronauts[i]->x++;
            }
            else if (astronauts[i]->x > N) {
                astronauts[i]->x--;
            }
        }
    }
}

// determines whether an impostor can kill a target astronaut
bool imp_kill_target(struct Impostor* imp, int target_idx, struct Astronaut** astronauts) {
    if (astronauts[target_idx]->is_alive &&
        (astronauts[target_idx]->x == imp->x && astronauts[target_idx]->y == imp->y)) {
        return true;
    }

    return false;
}

// determines whether an astronaut can witness the killing of another astronaut by an impostor
bool witness(struct Astronaut* astro, struct Impostor* imp, struct Astronaut** astronauts) {
    // if the astronaut witness the impostor and the target astronaut, return true
    for (int i = 0; i < astro_count; i++) {
        if (((abs(astro->x - imp->x) == 1) && (abs(astro->y - imp->y) == 1)) ||
            ((abs(astro->x - imp->x) == 0) && (abs(astro->y - imp->y) == 1)) ||
            ((abs(astro->x - imp->x) == 1) && (abs(astro->y - imp->y) == 0)))
        {
            if ((imp->is_alive) && (astronauts[i]->is_alive) && (astro != astronauts[i]) &&
                (imp->x == astronauts[i]->x && imp->y == astronauts[i]->y))
            {
                return true;
            }
            
        }
    }
   

    // if the astronaut do not witness the impostor and the target astronaut, return false
    return false;
}

// advances the game to the next state
void next_state(struct People* people) {
    // move the astronauts
    move_astronauts(people->astronauts);

    // check if any astronaut was killed by an impostor
    for (int i = 0; i < astro_count; i++) {
        for (int j = 0; j < imp_count; j++) {
            // if the impostor can kill the astronaut, kill the astronaut and break out of the inner loop
            if (imp_kill_target(people->impostors[j], i, people->astronauts)) {
		people->astronauts[i]->is_alive = false;
                dead_astro_count++;
                break;
            }
        }
    }
    // check if any impostor was killed by an astronaut
    for (int i = 0; i < astro_count; i++) {
        for (int j = 0; j < imp_count; j++) {
            // if the astronaut witnessed the impostor killing another astronaut, kill the impostor
            if (witness(people->astronauts[i], people->impostors[j], people->astronauts)) {
                people->impostors[j]->is_alive = false;
                dead_imp_count++;
                break;
            }
        }
    }
}

// updates the game status
void update_game_status(struct People* people) {
    if (dead_imp_count == imp_count) {
        // if all the impostors are dead, the astronauts have won
        g_stat = Victory;
    }
    else if (dead_astro_count == astro_count) {
        // if all the astronauts are dead, the impostors have won
        g_stat = Defeat;
    }
    else {
        // if there are still both alive astronauts and impostors, the game continues
        g_stat = Continue;
    }
}

// reads the initial state of the game from a file
struct People* read_from_file(const char* filename) {
    // open the file for reading
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Failed to open file: %s\n", filename);
        exit(1);
    }

    // read the number of astronauts and impostors and
    //the dimension parameter N and number of iterations to simulate
    fscanf(file, "%d\n%d\n%d\n%d\n", &astro_count, &imp_count, &N, &iteration);

    // initialize the people structure
    struct People* people = malloc(sizeof(struct People));
    people->astronauts = malloc(astro_count * sizeof(struct Astronaut*));
    people->impostors = malloc(imp_count * sizeof(struct Impostor*));

    // read the initial coordinates of the astronauts
    for (int i = 0; i < astro_count; i++) {
        // create an astronaut with the given coordinates
        // add the astronaut to the list
        people->astronauts[i] = malloc(sizeof(struct Astronaut));
        fscanf(file, "%d,%d & ", &(people->astronauts[i]->x), &(people->astronauts[i]->y));
        if (astro_count == i - 1) {
            fscanf(file, "%d,%d\n", &(people->astronauts[i]->x), &(people->astronauts[i]->y));
        }
        people->astronauts[i]->is_alive = true;
        
    }

    // read the initial coordinates of the impostors
    for (int i = 0; i < imp_count; i++) {
        // create an impostor with the given coordinates add the impostor to the list
        people->impostors[i] = malloc(sizeof(struct Impostor));
        fscanf(file, "%d,%d & ", &(people->impostors[i]->x), &(people->impostors[i]->y));
        if (imp_count == i - 1) {
            fscanf(file, "%d,%d", &(people->impostors[i]->x), &(people->impostors[i]->y));
        }
        people->impostors[i]->is_alive = true;

    }

    // close the file
    fclose(file);

    return people;
}

  

// writes the current state of the game to a file
void write_a_file(char* in_fname, struct People* people, int cur_iteration) {
    // create the output filename by appending the current iteration to the input filename
    
    // Declare the out_fname variable as a pointer to a char pointer
    char** out_fname;

    // Dynamically allocate memory for the out_fname variable
    out_fname = malloc(sizeof(char*));

    // Pass the address of out_fname to asprintf
    if (asprintf(out_fname, "%s_out_%d", in_fname, cur_iteration) == -1) {
        printf("Error creating output file name!\n");
        exit(1);
    } 

    // open the file for writing
    FILE* file = fopen(*out_fname, "w");

    if (file == NULL) {
        printf("Failed to open file: %s\n", out_fname);
        exit(1);
    }

    // write the number of dead impostors
    fprintf(file, "%d\n", dead_imp_count);

    // write the number of alive impostors
    fprintf(file, "%d\n", imp_count - dead_imp_count);

    // write the number of dead astronauts
    fprintf(file, "%d\n", dead_astro_count);

    // write the number of alive astronauts
    fprintf(file, "%d\n", astro_count - dead_astro_count);

    // write the current game status
    switch (g_stat) {
    case Defeat:
        fprintf(file, "Defeat\n");
        break;
    case Victory:
        fprintf(file, "Victory\n");
        break;
    case Continue:
        fprintf(file, "Continue\n");
        break;
    }

    // write the status of the astronauts
    for (int i = 0; i < astro_count; i++) {
        fprintf(file, "%d, %d, ", people->astronauts[i]->x, people->astronauts[i]->y);
        if (people->astronauts[i]->is_alive) {
            fprintf(file, "Alive");
        }
        else {
            fprintf(file, "Dead");
        }
        fprintf(file, " & ");
    }
    // replace the final " & " with a newline
    fseek(file, -3, SEEK_CUR);
    fputs("\n", file);

    // write the status of the impostors
    for (int i = 0; i < imp_count; i++) {
        fprintf(file, "%d, %d, ", people->impostors[i]->x, people->impostors[i]->y);
        if (people->impostors[i]->is_alive) {
            fprintf(file, "Alive");
        }
        else {
            fprintf(file, "Dead");
        }
        if (i + 1 != imp_count) {
        fprintf(file, " & ");
	}
	else{
	fprintf(file, "\n");
	}
    }
 

    // close the file
    fclose(file);
}
int main(int argc, char** argv) {
        int cur_iteration = 0;
        char* filename = argv[1];
        g_stat = Continue;

        struct People* people = read_from_file(filename);

        print_state(people);
        write_a_file(filename, people, cur_iteration);

        for (cur_iteration = 1; cur_iteration < iteration; ++cur_iteration) {
            next_state(people);
            update_game_status(people);
            print_state(people);
            write_a_file(filename, people, cur_iteration);

            if (g_stat == Victory || g_stat == Defeat) {
                break;
            }
        }

        return 0;
}
