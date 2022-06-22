#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#define N 1000
//#define CLOCKS_PER_SEC 1000
#define EJECUCIONES 5

int I = 1200; // Esto es para el numero de iteraciones

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

void mostrar_matriz(celda **matriz){
    celda c;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
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


void inicializar(celda **matriz){
    celda c;
    float fherida_abierta;
    float estado;
    float tipo_edad;
    int i;
    int j;  

    #pragma omp parallel for shared(matriz) private(i,c) num_threads(1) 
    for (i = 0; i < N; i++) {
        #pragma omp parallel for shared(matriz) private(j,c) num_threads(1)
        for (j = 0; j < N; j++) {
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

int calcular_vecinos(celda **matriz, int i, int j){
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
    if(!(j+1>N-1)){
        if(matriz[i][j+1].estado == 2){
            result = result + 1;
        }

        if(!(j+2>N-1)){
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

        if(!(j+1>N-1)){ // esquina superior derecha
            if(matriz[i-1][j+1].estado == 2){
                result = result + 1;
            }
        }

    }

    // Acá hago lo mismo pero con la parte de abajo
    if(!(i+1>N-1)){
        if(matriz[i+1][j].estado == 2){
            result = result + 1;
        }

        // Otro vecino de abajo
        if(!(i+2>N-1)){
            if(matriz[i+2][j].estado == 2){
                result = result + 1;
            }
        }

        if(!(j-1<0)){ // esquina inferior izquierda
            if(matriz[i+1][j-1].estado == 2){
                result = result + 1;
            }
        }

        if(!(j+1>N-1)){ // esquina inferior derecha
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

celda ** function_crear_matriz(){
    celda **matriz;
    matriz= (celda **)malloc (N*sizeof(celda *));
    celda *mf;
    mf = (celda *) malloc(N*N*sizeof(celda));

    for (int i=0; i<N; i++) {
        matriz[i] = mf + i*N;
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

void functionPrincipal(celda ***matriz,celda ***matriz_resultado,int iteraciones){
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

    while (contador != iteraciones) {
        //printf("Numero de iteracion:%d\n", contador);
        //printf("\n");

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
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
                    num_vecinos = calcular_vecinos((*matriz), i, j);
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
        //mostrar_matriz(*matriz_resultado);
        puntero = (*matriz);
        (*matriz) = (*matriz_resultado);
        (*matriz_resultado) = puntero;
        contador++;
    }

}

int main() {
    // Variables para el tiempo
    clock_t start, finish;
    double  duration;
    double tiempo_total = 0;
    double promedios;

    // Creo matrices
    celda ** matriz = function_crear_matriz();
    celda ** matriz_resultado = function_crear_matriz();

    // Ejecucion del programa
    //srand(time(NULL)); // Semilla para los numeros random.

    for(int i=0;i<EJECUCIONES;i++){
        start = clock();
        inicializar(matriz);
        //mostrar_matriz(matriz);
	set_seed_random(EJECUCIONES);
        functionPrincipal(&matriz,&matriz_resultado,I);
        finish = clock();
        duration = (double)(finish - start) / (clock_t)1000000;
        printf( "Tiempo de la °%d ejecucion (segundos):%lf\n\n",i+1,duration);
        tiempo_total+= duration;
        duration = 0;
    }
    printf("\n");
    printf("########  Tiempo total de la ejecución: %lf  ########\n",tiempo_total);
    promedios = tiempo_total/EJECUCIONES;
    printf("########  Tiempo promedio:%lf  ########\n",promedios);

    // Libero memoria para las matrices.
    function_liberar_memoria(matriz);
    function_liberar_memoria(matriz_resultado);

	return 0;
}

