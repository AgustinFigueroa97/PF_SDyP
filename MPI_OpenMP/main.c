#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

//#define CLOCKS_PER_SEC 1000
#define EJECUCIONES 5
#define SEMANAS 1200 // Esto es para el numero de iteraciones

int n = 1500; // Por este parametro me indicaran la dimension de la matriz

/*Dejo una matriz de 3x3 para pruebas unicamente, en el clauster se probara el programa con las correspondientes dimensiones*/

// Defino la estructura para la celda
typedef struct
{
    int estado; // 0 --> blanco, 1 --> azul, 2 --> rojo, 3 --> naranja, 4 -->verde
    int t_edad; // 0 --> joven, 1 --> adulto, 2 --> viejo
    int edad; // Esto lo agrego para mantener el numero de ciclo de cada arbol
    int fherida_abierta; // Esto es un flag que indicará si tiene o no heridas abiertas.
    int tiempo_contagio; // indicará el tiempo de contagio de un arbol.
}celda;

void mostrar_celda(celda c){
    printf("Estado:%d\n",c.estado);
    printf("Tipo de Edad:%d\n",c.t_edad);
    printf("Edad:%d\n",c.edad);
    printf("Tiempo de Contagio:%d\n",c.tiempo_contagio);
    printf("Herida:%d\n",c.fherida_abierta);
    printf("\n");
}

void mostrar_matriz(celda **matriz, int desde,int hasta){
    celda c;
    for (int i = desde; i < hasta; i++) {
        for (int j = 0; j < n; j++) {
            c = matriz[i][j];
            mostrar_celda(c);
        }
        printf("\n");
    }
}

float funcionRandomIntervalo(int random, int a, int b){
    return (float)(random %(b-a+1) + a)/100.0f;
}

int function_heridas(int edad, float fherida){
    int result;
    if(edad == 0 && fherida <= 0.23 ){
        result = 1;
    }else if(edad == 1 && fherida <=0.08){
        result = 1;
    }else if(edad == 2 && fherida <=0.37){
        result = 1;
    }else{
        result = 0;
    }
    return result;
}


int generadorRandomsEdad(int random,int a, int b) {
    int ret = a + (b - a) * funcionRandomIntervalo(random,0,100);
    return ret;
}


void inicializar(celda **matriz, int desde, int hasta){
    celda c;
    float fherida_abierta;
    float estado;
    float tipo_edad;
    int i;
    int j; 

    #pragma omp parallel for shared(matriz) private(i,c) num_threads(1) 
    for (i = desde; i < hasta; i++) {
        #pragma omp parallel for shared(matriz) private(j,c) num_threads(1)
        for (j = 0; j < n; j++) {
            c.edad = 0;
            
            //Vamos a tirar random para el estado
            estado = funcionRandomIntervalo(rand(),0,100);
            if(estado <=0.65){
                c.estado = 4; // El arbol esta sano
		c.tiempo_contagio = 0;
            }else if(estado >0.65 && estado <=0.70){
                c.estado = 2; // arbol enfermo con sintomas
		c.tiempo_contagio = generadorRandomsEdad(rand(),0,7); 
            }else if(estado >0.70 && estado <=0.80){
                c.estado = 3; // infectado sin sintomas
		c.tiempo_contagio = generadorRandomsEdad(rand(),0,7); 
            }else{
                c.estado = 1; // arbol con tratamiento
		c.tiempo_contagio = generadorRandomsEdad(rand(),4,7); 
            }

            tipo_edad = funcionRandomIntervalo(rand(),0,100);
            if(tipo_edad <=0.30){
               c.t_edad = 0;  // arbol joven, asumo como que recien nace jaja
	       c.edad = generadorRandomsEdad(rand(),1,143);
            }else if(tipo_edad > 0.30 && tipo_edad <=0.80){
                c.t_edad = 1; // arbol adulto, a partir de las 144 semanas es adulto
                c.edad = generadorRandomsEdad(rand(),144,1679); 
            }else{
                c.t_edad = 2; // arbol viejo
                c.edad = generadorRandomsEdad(rand(),1680,2000); 
            }
            fherida_abierta = funcionRandomIntervalo(rand(),0,100);
            c.fherida_abierta = function_heridas(c.t_edad,fherida_abierta);
            matriz[i][j] = c;
        }
    }

}

int calcular_vecinos(celda **matriz, int i, int j,int borde){
    int result = 0;
// Calculo el vecino izquierdo
    if(!(j-1<0)){
        if(matriz[i][j-1].estado == 2){ // Si su vecino contagia lo cuento como vecino que contagia.
            result = result + 1;
        }

        if(!(j-2<0)){
            if(matriz[i][j-2].estado == 2){
                result = result + 1;
            }
        }
    }

    // Calculo el vecino derecho
    if(!(j+1>n-1)){
        if(matriz[i][j+1].estado == 2){
            result = result + 1;
        }

        if(!(j+2>n-1)){
            if(matriz[i][j+2].estado == 2){
                result = result + 1;
            }
        }
    }

    // Con este if verifico si se puede calcular arriba, en caso de que no se pueda
    // ni siquiera me fijo sus esquinas superiores e inferiores
    if(!(i-1<0)){
        if(matriz[i-1][j].estado == 2){
            result = result + 1;
        }

        // otro vecino de arriba
        if(!(i-2<0)){
            if(matriz[i-2][j].estado == 2){
                result = result + 1;
            }
        }

        if(!(j-1<0)){	// esquina superior izquierda
            if(matriz[i-1][j-1].estado == 2){
                result = result + 1;
            }
        }

        if(!(j+1>n-1)){ // esquina superior derecha
            if(matriz[i-1][j+1].estado == 2){
                result = result + 1;
            }
        }

    }

    // Acá hago lo mismo pero con la parte de abajo
    if(!(i+1>borde-1)){
        if(matriz[i+1][j].estado == 2){
            result = result + 1;
        }

        // Otro vecino de abajo
        if(!(i+2>borde-1)){
            if(matriz[i+2][j].estado == 2){
                result = result + 1;
            }
        }

        if(!(j-1<0)){ // esquina inferior izquierda
            if(matriz[i+1][j-1].estado == 2){
                result = result + 1;
            }
        }

        if(!(j+1>n-1)){ // esquina inferior derecha
            if(matriz[i+1][j+1].estado == 2){
                result = result + 1;
            }
        }

    }
    //printf("Valor del resultado del vecino:%d\n\n",result);
    return result;
}

float funcion_susceptibilidad(int t_edad, int herida){
    float result = 0;
    if(t_edad == 0) {
        result = 0.35;
    } else if(t_edad == 1){
        result = 0.17;
    }else{
        result = 0.63;
    }

    if(herida){
        result += 0.15;
    }
    return result;
}

celda ** function_crear_matriz(int filas,int columnas){
    celda **matriz;
    matriz= (celda **)malloc (filas*sizeof(celda *));
    celda *mf;
    mf = (celda *) malloc(filas*columnas*sizeof(celda));

    for (int i=0; i<filas; i++) {
        matriz[i] = mf + i*columnas;
    }
    return matriz;
}

void function_liberar_memoria(celda **matriz){
	free((void*)matriz);
}

void set_seed_random(int id_process){
	srand(time(NULL));
	srand(rand()+(id_process+7)*13);
}

void mostrar_matriz_color(celda** matriz,int desde, int hasta) { ///EL COLOR SOLO FUNCIONA EN LINUX, EN WINDOWS HAY QUE COMENTAR LOS PRINTS CON CODIGO DE ESCAPE  EJ: \033
    for (int i = desde; i < hasta; i++) {
        for (int j = 0; j < n; j++) {
            char estado='-';
            switch (matriz[i][j].estado) {
                case 2:{
                    estado='R';
                    printf("\033[0;31m"); 
                    break;
                }
                case 1:{
                    estado='A';
                    printf("\033[0;34m"); 
                    break;

                }
                case 0:{
                    estado='B';
                    printf("\033[0;37m"); 
                    break;

                }
                case 3:{
                    estado='N';
                    printf("\033[0;33m"); 
                    break;

                }
                case 4:{
                    estado='V';
                    printf("\033[0;32m"); 
                    break;
                }
            }
            if(matriz[i][j].fherida_abierta){
                printf("|{%c}|",estado);
            }else{
                printf("|[%c]|",estado);
            }
            printf("\033[0m");//Reestrablecer color
            if(j==n-1){
                printf("\n");
            }
        }
    }
}


// Esta funcion deberia de corroborarla, en el sentido en el que creo que esta es la forma correcta de como 
// Se deberia de liberar la memoria. 
/*
void function_liberar_memoria_2(celda **matriz, int fila){
    for(int i = 0; i < fila; i++)
        free(matriz[i]);
    free(matriz);
}
*/


MPI_Datatype crearTipo(){
	// Create the datatype
    MPI_Datatype tipo_celda;
    int longitud[5] = {1, 1, 1, 1, 1};

    MPI_Aint desplazamiento[5];
    celda celda_ficticia;
    MPI_Aint direccion_base;
    MPI_Get_address(&celda_ficticia, &direccion_base);
    MPI_Get_address(&celda_ficticia.estado, &desplazamiento[0]);
    MPI_Get_address(&celda_ficticia.t_edad, &desplazamiento[1]);
    MPI_Get_address(&celda_ficticia.edad, &desplazamiento[2]);
    MPI_Get_address(&celda_ficticia.fherida_abierta, &desplazamiento[3]);
    MPI_Get_address(&celda_ficticia.tiempo_contagio, &desplazamiento[4]);

    desplazamiento[0] = MPI_Aint_diff(desplazamiento[0], direccion_base);
    desplazamiento[1] = MPI_Aint_diff(desplazamiento[1], direccion_base);
    desplazamiento[2] = MPI_Aint_diff(desplazamiento[2], direccion_base);
    desplazamiento[3] = MPI_Aint_diff(desplazamiento[3], direccion_base);
    desplazamiento[4] = MPI_Aint_diff(desplazamiento[4], direccion_base);

    MPI_Datatype tipos[5] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Type_create_struct(5, longitud, desplazamiento, tipos, &tipo_celda);
    MPI_Type_commit(&tipo_celda);
	return tipo_celda; 
}



void functionPrincipal(celda ***matriz,celda ***matriz_resultado,int desde, int hasta,int borde){
    int contador=0;
    int estado = 0;
    int num_vecinos;
    float p_vecinos_con_sintomas = 0;
    float susceptibilidad = 0;
    float p_contagio = 0;
    float fherida_abierta;
    float random;
    int semana_12=0;
    celda **puntero;
    int i; 
    int j; 
    celda aux;
    int div1,div2; 
    div1 = floor(n/3);
    div2 = floor(n/4); 

    #pragma omp parallel for schedule(dynamic,div1) private(i) num_threads(2)
    for (int i = desde; i < hasta; i++) {
	#pragma omp parallel for shared(matriz,matriz_resultado) schedule(dynamic,div2) private(j,aux) num_threads(4)
        for (int j = 0; j < n; j++) {
		
             	celda aux;
                aux.estado = (*matriz)[i][j].estado;
                aux.t_edad = (*matriz)[i][j].t_edad;
                aux.edad = (*matriz)[i][j].edad;
                aux.fherida_abierta = (*matriz)[i][j].fherida_abierta;
                aux.tiempo_contagio = (*matriz)[i][j].tiempo_contagio;

                // Se realizan los calculos sobre la matriz anterior y asi..
               aux.fherida_abierta = function_heridas((*matriz)[i][j].t_edad,funcionRandomIntervalo(rand(), 0, 100));
               aux.edad += 1; // Todos los arboles por ciclo van aumentando su edad.

                if ((*matriz)[i][j].t_edad == 0) {
                    if ((*matriz)[i][j].edad >= 144) {
                        aux.t_edad = 1;
                    }
                } else if ((*matriz)[i][j].t_edad == 1) {
                    if ((*matriz)[i][j].edad >= 1680) {
                        aux.t_edad = 2;
                    }
                }

                estado = (*matriz)[i][j].estado;

                if (estado == 0) { // es un arbol podado
                    semana_12 += 1; // voy contando la semana
                    if (semana_12 >= 12) { // PARA PRUEBA VAMOS A USAR 4 SEMANAS
                        aux.estado = 4; // cuando llego la semana 12 se curo.
                        aux.tiempo_contagio = 0; // Deja de contagiar porque se cura.
                    }
                } else if (estado == 1) {// es un arbol Enfermo con tratamiento antifungico.
                    aux.tiempo_contagio += 1;
                    if ((*matriz)[i][j].tiempo_contagio >= 8) {
                        // Aca podría pasar a 3 poibles estados mas
                        random = funcionRandomIntervalo(rand(), 0, 100);
                        //printf("Estado 1 valor del random:%f\n",random);
                        if ((*matriz)[i][j].t_edad == 0) { // Si es un arbol joven
                            if (random < 0.03) { // si entra aca es porque lo podan
                                aux.estado = 0;
                            } else {
                                aux.estado = 4; // no lo podan sino que se curo
                            }
                            aux.tiempo_contagio = 0; // Ya sea que lo poden o curen el arbol ya no cantagia
                        } else if ((*matriz)[i][j].t_edad == 1) {
                            if (random < 0.15) {
                                aux.estado = 0;
                            } else {
                                aux.estado = 4;
                            }
                            aux.tiempo_contagio = 0; // Lo mismo aca ya no contagia
                        } else {
                            if (random < 0.53) {
                                aux.t_edad = 0; // ahora es joven
                                aux.estado = 4; // ahora esta sano
                                aux.edad = 48; // Tiene un año
                                aux.tiempo_contagio = 0; // asumo que ahora no esta contagiado de nada.
                                aux.fherida_abierta = 0; // asumo como es nuevo no esta lastimado.
                            }
                        }
                    }

                } else if (estado == 2) { // es un arbol enfermo con sintomas visibles
                    random = funcionRandomIntervalo(rand(), 0, 100);
                    //printf("Estado 2 valor del random:%f\n",random);
                    if (random < 0.85) {
                        aux.estado = 1;
                        aux.tiempo_contagio = 0;
                    }
                } else if (estado == 3) { // es un arbol infectado con esporas sin sintomas visibles
                    aux.tiempo_contagio = aux.tiempo_contagio+ 1;
                    if ((*matriz)[i][j].tiempo_contagio >= 3) {
                        aux.estado = 2;
                    }
                } else { // es un arbol sano.
                    num_vecinos = calcular_vecinos((*matriz), i, j,borde);
                    p_vecinos_con_sintomas = (float) num_vecinos / 12;
                    //printf("Porcentaje de vecinos con sintomas:%f\n",p_vecinos_con_sintomas);
                    susceptibilidad = funcion_susceptibilidad((*matriz)[i][j].t_edad, (*matriz)[i][j].fherida_abierta);
                    p_contagio = (p_vecinos_con_sintomas + susceptibilidad) * 0.60 + 0.07;
                    random = funcionRandomIntervalo(rand(), 0, 100);
                    //printf("Estado 4 probabilidad de contagio:%f\n",p_contagio);
                    //printf("Estado 4 valor del random:%f\n",random);
                    if (p_contagio < random) {
                        aux.estado = 3;
                    }
                }
				
                (*matriz_resultado)[i][j].estado = aux.estado;
                (*matriz_resultado)[i][j].t_edad = aux.t_edad;
                (*matriz_resultado)[i][j].edad = aux.edad;
                (*matriz_resultado)[i][j].fherida_abierta = aux.fherida_abierta;
                (*matriz_resultado)[i][j].tiempo_contagio = aux.tiempo_contagio;
				
             	
        }
    }

    //printf("\n");
    //mostrar_matriz(*matriz_resultado,0,n);
    puntero = (*matriz);
    (*matriz) = (*matriz_resultado);
    (*matriz_resultado) = puntero;

}

int main(int argc, char **argv) {
// Variables para el tiempo
    clock_t start, finish;
    double duration;
    double tiempo_total=0;
    int id_process, num_process;
    MPI_Status status;
    int division;
    double promedio;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id_process);
    MPI_Comm_size(MPI_COMM_WORLD, &num_process);
	celda **sub_matriz;
	celda **sub_matriz_resultado;
	MPI_Datatype tipo_celda = crearTipo();

    // Se realiza la division de los procesos
    division = floor(n / num_process);

    // Se realiza el pedido de memoria. 
    if(id_process == 0 || id_process == num_process -1){
            sub_matriz = function_crear_matriz(division+2,n);
            sub_matriz_resultado = function_crear_matriz(division+2,n);
    }else{
            sub_matriz = function_crear_matriz(division+4,n);
            sub_matriz_resultado = function_crear_matriz(division+4,n);
    }

	for(int i=0;i<EJECUCIONES;i++){	
		set_seed_random(id_process);
		
			if(id_process == 0){
                start=clock();
                inicializar(sub_matriz,0,division);
                //mostrar_matriz_color(sub_matriz,0,division); 
            }else{
                inicializar(sub_matriz,2,division+2);
                //mostrar_matriz_color(sub_matriz,2,division+2); 
            }
			

		// Sincronizacion, politica los pares enviar primero y los impares reciben primero 
		for(int j=0;j< SEMANAS;j++){

		  if (id_process % 2 == 0) { // Es decir si es par 
				if (id_process == 0) { // Ademas de ser par es el proceso 0 
					MPI_Send(&(sub_matriz[division-2][0]), 1*n, tipo_celda, id_process+1, 0, MPI_COMM_WORLD);
					MPI_Send(&(sub_matriz[division-1][0]), 1*n, tipo_celda, id_process+1, 0, MPI_COMM_WORLD);
					MPI_Recv(&(sub_matriz[division][0]), 1*n, tipo_celda, id_process+1, 0, MPI_COMM_WORLD, &status);
					MPI_Recv(&(sub_matriz[division+1][0]), 1*n, tipo_celda, id_process+1, 0, MPI_COMM_WORLD, &status);
					functionPrincipal(&sub_matriz,&sub_matriz_resultado,0,division,division+2);
				}else{ // Si son los intermedios 
					if(id_process == num_process-1){ // Ahora ademas de ser el intermedio, ese intermedio es el ultimo proceso
						MPI_Send(&(sub_matriz[2][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD);
						MPI_Send(&(sub_matriz[3][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD);
						MPI_Recv(&(sub_matriz[0][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD, &status);
						MPI_Recv(&(sub_matriz[1][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD, &status);
						functionPrincipal(&sub_matriz,&sub_matriz_resultado,2,division+2,division+2);					
					}else{ // Si esos intermedios no es el ultimo proceso 
						MPI_Send(&(sub_matriz[2][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD);
						MPI_Send(&(sub_matriz[3][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD);
						MPI_Send(&(sub_matriz[division][0]), 1*n, tipo_celda, id_process+1, 0, MPI_COMM_WORLD);
						MPI_Send(&(sub_matriz[division+1][0]), 1*n, tipo_celda, id_process+1, 0, MPI_COMM_WORLD);
						MPI_Recv(&(sub_matriz[0][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD, &status);
						MPI_Recv(&(sub_matriz[1][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD, &status);
						MPI_Recv(&(sub_matriz[division+2][0]), 1*n, tipo_celda, id_process+1, 0, MPI_COMM_WORLD, &status);
						MPI_Recv(&(sub_matriz[division+3][0]), 1*n, tipo_celda, id_process+1, 0, MPI_COMM_WORLD, &status);
						functionPrincipal(&sub_matriz,&sub_matriz_resultado,2,division+2,division+4);	
					}				
				}    
			 }else { // Ahora si el proceso es impar 
				if (id_process==num_process-1) { // Y si ademas de ser impar es el ultimo proceso 
					MPI_Recv(&(sub_matriz[0][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD, &status);
					MPI_Recv(&(sub_matriz[1][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD, &status);
					MPI_Send(&(sub_matriz[2][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD);
					MPI_Send(&(sub_matriz[3][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD);
					//printf("estoy por procesar en impares %d\n",id_process);
					functionPrincipal(&sub_matriz,&sub_matriz_resultado,2,division+2,division+2);	
				} else {
					MPI_Recv(&(sub_matriz[0][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD, &status);
					MPI_Recv(&(sub_matriz[1][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD, &status);
					MPI_Recv(&(sub_matriz[division+2][0]), 1*n, tipo_celda, id_process+1, 0, MPI_COMM_WORLD, &status);
					MPI_Recv(&(sub_matriz[division+3][0]), 1*n, tipo_celda, id_process+1, 0, MPI_COMM_WORLD, &status);
					MPI_Send(&(sub_matriz[2][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD);
					MPI_Send(&(sub_matriz[3][0]), 1*n, tipo_celda, id_process-1, 0, MPI_COMM_WORLD);
					MPI_Send(&(sub_matriz[division][0]), 1*n, tipo_celda, id_process+1, 0, MPI_COMM_WORLD);
					MPI_Send(&(sub_matriz[division+1][0]), 1*n, tipo_celda, id_process+1, 0, MPI_COMM_WORLD);
					functionPrincipal(&sub_matriz,&sub_matriz_resultado,2,division+2,division+4);	
				}   
			}
			//printf("Complete una semana %d\n",id_process);
            
			
		} // Esto cierra el for de las semanas. 
        if(id_process==0){
            finish=clock();
            duration=(double)(finish-start)/ (clock_t)1000000;
            printf( "Tiempo de la °%d ejecucion (segundos):%lf\n\n",i+1,duration);
            tiempo_total+= duration;
            duration = 0;
        }
        MPI_Barrier(MPI_COMM_WORLD);
		 
	} // Aca cierra el for de ejecuciones

    if(id_process==0){
        printf("\n");
        printf("########  Tiempo total de la ejecución: %lf  ########\n",tiempo_total);
        promedio = tiempo_total/EJECUCIONES;
        printf("########  Tiempo promedio:%lf  ########\n",promedio);
    }
    MPI_Barrier(MPI_COMM_WORLD);
	
    /*LIBERAR MEMORIA DE LAS SUBMATRICES IMPOOOOOOOOOOORTANTE*/
	function_liberar_memoria(sub_matriz);
	function_liberar_memoria(sub_matriz_resultado);
		
    MPI_Finalize();

}
