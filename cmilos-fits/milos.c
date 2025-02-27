
//    _______             _______ _________ _        _______  _______ 
//   (  ____ \           (       )\__   __/( \      (  ___  )(  ____ \
//   | (    \/           | () () |   ) (   | (      | (   ) || (    \/
//   | |         _____   | || || |   | |   | |      | |   | || (_____ 
//   | |        (_____)  | |(_)| |   | |   | |      | |   | |(_____  )
//   | |                 | |   | |   | |   | |      | |   | |      ) |
//   | (____/\           | )   ( |___) (___| (____/\| (___) |/\____) |
//   (_______/           |/     \|\_______/(_______/(_______)\_______)
//     
//
// CMILOS v0.9 (2015)
// RTE INVERSION C code for SOPHI (based on the ILD code MILOS by D. Orozco)
// juanp (IAA-CSIC)
//
// How to use:
//
//  >> milos NLAMBDA MAX_ITER CLASSICAL_ESTIMATES [FWHM DELTA NPOINTS] profiles_file.txt > output.txt
//
//   NLAMBDA number of lambda of input profiles
//   MAX_ITER of inversion
//   CLASSICAL_ESTIMATES use classical estimates? 1 yes, 0 no
//   [FWHM DELTA NPOINTS] use convolution with a gaussian? if the tree parameteres are defined yes, else no. Units in A. NPOINTS has to be odd.
//   profiles_file.txt name of input profiles file
//   output.txt name of output file
//
//

#include "defines.h"

#include "nrutil.h"
#include "svdcmp.c"
#include "svdcordic.c"
//#include "tridiagonal.c"
#include "convolution.c"

//#include <math.h>
//#include <fitsio.h>
#include <string.h>

float pythag(float a, float b);

void weights_init(int nlambda,double *sigma,PRECISION *weight,int nweight,PRECISION **wOut,PRECISION **sigOut,double noise);

int check(Init_Model *Model);
int lm_mils(Cuantic *cuantic,double * wlines,int nwlines,double *lambda,int nlambda,PRECISION *spectro,int nspectro, 
		Init_Model *initModel, PRECISION *spectra,int err,double *chisqrf, int *iterOut,
		double slight, double toplim, int miter, PRECISION * weight,int nweight, int * fix, 
		PRECISION *sigma, double filter, double ilambda, double noise, double *pol,
		double getshi,int triplete);

int mil_svd(PRECISION *h,PRECISION *beta,PRECISION *delta);

int multmatrixIDL(double *a,int naf,int nac, double *b,int nbf,int nbc,double **resultOut,int *fil,int *col);
int multmatrix_transposeD(double *a,int naf,int nac, double *b,int nbf,int nbc,double *result,int *fil,int *col);
int multmatrix3(PRECISION *a,int naf,int nac,double *b,int nbf,int nbc,double **result,int *fil,int *col);
double * leeVector(char *nombre,int tam);
double * transpose(double *mat,int fil,int col);

double total(double * A, int f,int c);
int multmatrix(PRECISION *a,int naf,int nac, PRECISION *b,int nbf,int nbc,PRECISION *result,int *fil,int *col);
int multmatrix2(double *a,int naf,int nac, PRECISION *b,int nbf,int nbc,double **result,int *fil,int *col);

int covarm(PRECISION *w,PRECISION *sig,int nsig,PRECISION *spectro,int nspectro,PRECISION *spectra,PRECISION  *d_spectra,
		PRECISION *beta,PRECISION *alpha);

int CalculaNfree(PRECISION *spectro,int nspectro);

double fchisqr(PRECISION * spectra,int nspectro,PRECISION *spectro,PRECISION *w,PRECISION *sig,double nfree);

void AplicaDelta(Init_Model *model,PRECISION * delta,int * fixed,Init_Model *modelout);
void FijaACeroDerivadasNoNecesarias(PRECISION * d_spectra,int *fixed,int nlambda);
void reformarVector(PRECISION **spectro,int neje);
void spectral_synthesis_convolution();
void response_functions_convolution();

void estimacionesClasicas(PRECISION lambda_0,double *lambda,int nlambda,PRECISION *spectro,Init_Model *initModel);

#define tiempo(ciclos) asm volatile ("rdtsc \n\t":"=A"(ciclos)) 
long long int c1,c2,cd,semi,c1a,c2a,cda;			//variables de 64 bits para leer ciclos de reloj
long long int c1total,c2total,cdtotal,ctritotal;


Cuantic* cuantic;   // Variable global, está hecho así, de momento,para parecerse al original
char * concatena(char *a, int n,char*b);

PRECISION ** PUNTEROS_CALCULOS_COMPARTIDOS;
int POSW_PUNTERO_CALCULOS_COMPARTIDOS;
int POSR_PUNTERO_CALCULOS_COMPARTIDOS;

PRECISION * gp1,*gp2,*dt,*dti,*gp3,*gp4,*gp5,*gp6,*etai_2;

//PRECISION gp4_gp2_rhoq[NLAMBDA],gp5_gp2_rhou[NLAMBDA],gp6_gp2_rhov[NLAMBDA];
PRECISION *gp4_gp2_rhoq,*gp5_gp2_rhou,*gp6_gp2_rhov;


PRECISION *dgp1,*dgp2,*dgp3,*dgp4,*dgp5,*dgp6,*d_dt;
PRECISION * d_ei,*d_eq,*d_eu,*d_ev,*d_rq,*d_ru,*d_rv;
PRECISION *dfi,*dshi;
PRECISION CC,CC_2,sin_gm,azi_2,sinis,cosis,cosis_2,cosi,sina,cosa,sinda,cosda,sindi,cosdi,sinis_cosa,sinis_sina;
PRECISION *fi_p,*fi_b,*fi_r,*shi_p,*shi_b,*shi_r;
PRECISION *etain,*etaqn,*etaun,*etavn,*rhoqn,*rhoun,*rhovn;
PRECISION *etai,*etaq,*etau,*etav,*rhoq,*rhou,*rhov;
PRECISION *parcial1,*parcial2,*parcial3;
PRECISION *nubB,*nupB,*nurB;
PRECISION **uuGlobalInicial;
PRECISION **HGlobalInicial;
PRECISION **FGlobalInicial;
PRECISION *perfil_instrumental;
PRECISION * G;	
int FGlobal,HGlobal,uuGlobal;

PRECISION *d_spectra,*spectra;

//Number of lambdas in the input profiles
int NLAMBDA = 0;

//Convolutions values
int NMUESTRAS_G	= 0;
PRECISION FWHM = 0;
PRECISION DELTA = 0;

int INSTRUMENTAL_CONVOLUTION = 0;
int CLASSICAL_ESTIMATES = 0;

int main(int argc,char **argv){
	
	double * wlines;
	int nwlines;
	double *lambda;
	int nlambda;
	PRECISION *spectro;
	int ny,i,j;
	Init_Model initModel;
	int err;
	double chisqrf;
	int iter;
	double slight;
	double toplim;
	int miter;
	PRECISION weight[4]={1.,10.,10.,4.}; //{1.,1.,1.,1.};
	int nweight;

	// CONFIGURACION DE PARAMETROS A INVERTIR	
	//INIT_MODEL=[eta0,magnet,vlos,landadopp,aa,gamma,azi,B1,B2,macro,alfa]
	int fix[]={1.,1.,1.,1.,1.,1.,1.,1.,1.,0.,0.}; 
	//----------------------------------------------	
	
	double sigma[NPARMS];
	double vsig;
	double filter;
	double ilambda;
	double noise;
	double *pol;
	double getshi;	
	
	double dat[7]={CUANTIC_NWL,CUANTIC_SLOI,CUANTIC_LLOI,CUANTIC_JLOI,CUANTIC_SUPI,CUANTIC_LUPI,CUANTIC_JUPI};

	char *nombre,*input_iter;
	int Max_iter;

	struct img_t img;
	struct img_t mask;
	struct inv_t inv;

	//milos NLAMBDA MAX_ITER CLASSICAL_ESTIMATES[FWHM DELTA NPOINTS] perfil.txt
	
	if(argc!=5 && argc !=8 ){
		printf("milos: Error en el numero de parametros: %d .\n Pruebe: milos NLAMBDA MAX_ITER CLASSICAL_ESTIMATES [FWHM(in A) DELTA(in A) NPOINTS] perfil.txt\n",argc);
		return -1;
	}	
		
	NLAMBDA = atoi(argv[1]);
	
	input_iter = argv[2];
	Max_iter = atoi(input_iter);
	CLASSICAL_ESTIMATES = atoi(argv[3]);
	
	if(CLASSICAL_ESTIMATES!=0 && CLASSICAL_ESTIMATES != 1){
		printf("milos: Error in CLASSICAL_ESTIMATES parameter. 0 or 1 are valid values. Not accepted: \n",CLASSICAL_ESTIMATES);
		return -1;
	}
	
	if(argc ==5){		
		nombre = argv[4];
	}
	else{	
		INSTRUMENTAL_CONVOLUTION = 1;
		NMUESTRAS_G = atoi(argv[6]);
		FWHM = atof(argv[4]);
		DELTA = atof(argv[5]);
		nombre = argv[7];
	}

	
	//Generamos la gaussiana -> perfil instrumental			
	if(INSTRUMENTAL_CONVOLUTION){
		G=vgauss(FWHM,NMUESTRAS_G,DELTA);
		
		//if you wish to convolution with other instrumental profile you have to declare here and to asign it to "G"
	}

	
	cuantic=create_cuantic(dat);
	Inicializar_Puntero_Calculos_Compartidos();	

	toplim=1e-18;

	CC=PI/180.0;
	CC_2=CC*2;

	filter=0;
	getshi=0;
	nweight=4;

	nwlines=1;
	wlines=(double*) calloc(2,sizeof(double));
	wlines[0]=1;
	wlines[1]= CENTRAL_WL;
	
	vsig=NOISE_SIGMA; //original 0.001
	sigma[0]=vsig;
	sigma[1]=vsig;
	sigma[2]=vsig;
	sigma[3]=vsig;
	pol=NULL;

	noise=NOISE_SIGMA;
	ilambda=ILAMBDA;
	iter=0;
	miter=Max_iter;

	nlambda=NLAMBDA;
	lambda=calloc(nlambda,sizeof(double));
	spectro=calloc(nlambda*4,sizeof(PRECISION));

	/*
	// TODO: cfitsio file IO, classical estimates won't work with this for now
	FILE *fichero, *fichero_estimacioneClasicas;

	fichero= fopen(nombre,"r");
	if(fichero==NULL){   
		printf("Error de apertura, es posible que el fichero no exista.\n");
		printf("Milos: Error de lectura del fichero. ++++++++++++++++++\n");
		return 1;
	}

	char * buf;
	buf=calloc(strlen(nombre)+15,sizeof(char));
	buf = strcat(buf,nombre);
	buf = strcat(buf,"_CL_ESTIMATES");
	
	if(CLASSICAL_ESTIMATES){
		fichero_estimacioneClasicas= fopen(buf,"w");
		if(fichero_estimacioneClasicas==NULL){   
			printf("Error de apertura ESTIMACIONES CLASICAS, es posible que el fichero no exista.\n");
			printf("Milos: Error de lectura del fichero. ++++++++++++++++++\n");
			return 1;
		}		
	}
	*/
	int neje;
	double lin;	
	double iin,qin,uin,vin;
	int rfscanf;
	int contador;	

	int totalIter=0;

	contador=0;

	ReservarMemoriaSinteisisDerivadas(nlambda);

	//initializing weights
	PRECISION *w,*sig;
	weights_init(nlambda,sigma,weight,nweight,&w,&sig,noise);

	c2total=0;
	ctritotal=0;

	int nsub,indaux;
	indaux=0;

	init_img_struct(&img, nombre);
	init_inv_struct(&inv, &img);
	
	load_fits(&img, nombre);

	int x, y, xdim, ydim;
	xdim = img.xdim;
	ydim = img.ydim;

	for (x=0; x<xdim; x++){
		for (y=0; y<ydim; y++){

			// img.mask is 0 if pixel shall be excluded
			if (img.mask[y*ydim+x] == 0){
				inv.iter[y*ydim+x]	   = 0;
				inv.B[y*ydim+x]		   = 0;
				inv.gm[y*ydim+x]	   = 0;
				inv.az[y*ydim+x]	   = 0;
				inv.eta0[y*ydim+x]	   = 0;
				inv.dopp[y*ydim+x]	   = 0;
				inv.aa[y*ydim+x]	   = 0;
				inv.vlos[y*ydim+x]	   = 0;
				inv.alfa[y*ydim+x]	   = 0;
				inv.S0[y*ydim+x]	   = 0;
				inv.S1[y*ydim+x]	   = 0;
				inv.nchisqrf[y*ydim+x] = 0;
				continue;
			} 

			printf("\rProcessing column %d/%d. %.2f %%", x+1, xdim, 100*x/(float)xdim);
			fflush(stdout);
			
			neje=0;
			nsub=0;

			for (neje=0; neje<NLAMBDA; neje++){
				
			//while (neje<NLAMBDA && (rfscanf=fscanf(fichero,"%lf %le %le %le %le",&lin,&iin,&qin,&uin,&vin))!= EOF){
				lambda[nsub]=img.lambda[nsub];	
				//printf(" %f \n",lambda[nsub]);
				spectro[nsub]           = img.i[nsub][y*ydim+x];
				spectro[nsub+NLAMBDA]   = img.q[nsub][y*ydim+x];
				spectro[nsub+NLAMBDA*2] = img.u[nsub][y*ydim+x];
				spectro[nsub+NLAMBDA*3] = img.v[nsub][y*ydim+x];
				//if(x==150 && y==150)
				//	printf("%lf %lf %lf %lf %lf\n", lambda[nsub], spectro[nsub], spectro[nsub+NLAMBDA], spectro[nsub+NLAMBDA*2], spectro[nsub+NLAMBDA*3]);
				nsub++; //nsub and neje are counting the same?
				

			}
			
			//Initial Model		
		
			initModel.eta0 = INITIAL_MODEL_ETHA0;
			initModel.B    = INITIAL_MODEL_B; //200 700
			initModel.gm   = INITIAL_MODEL_GM;
			initModel.az   = INITIAL_MODEL_AZI;
			initModel.vlos = INITIAL_MODEL_VLOS; //km/s 0
			initModel.mac  = 0.0;
			initModel.dopp = INITIAL_MODEL_LAMBDADOPP;
			initModel.aa   = INITIAL_MODEL_AA;
			initModel.alfa = 1;							//0.38; //stray light factor
			initModel.S0   = INITIAL_MODEL_S0;
			initModel.S1   = INITIAL_MODEL_S1;
			
			/*
			if(CLASSICAL_ESTIMATES){
				estimacionesClasicas(wlines[1],lambda,nlambda,spectro,&initModel);
			
				//OJO CON LOS MENOS !!!

				if(isnan(initModel.B))
					initModel.B = 0;
				if(isnan(initModel.vlos))
					initModel.vlos = 0;
				if(isnan(initModel.gm))
					initModel.gm=0;
				if(isnan(initModel.az))
					initModel.az = 0;
				
				//Escribimos en fichero las estimaciones clásicas
				fprintf(fichero_estimacioneClasicas,"%e %e %e %e\n",initModel.B,initModel.gm,initModel.az,initModel.vlos);
			}	
			*/

			//inversion	
			lm_mils(cuantic,wlines,nwlines,lambda, nlambda,spectro,nlambda,&initModel,spectra,err,&chisqrf,&iter,slight,toplim,miter,
					weight,nweight,fix,sig,filter,ilambda,noise,pol,getshi,0);

			totalIter+=iter;

			//[contador;iter;B;GM;AZI;etha0;lambdadopp;aa;vlos;S0;S1;final_chisqr];
			inv.iter[y*ydim+x]	   = iter;

			inv.B[y*ydim+x]		   = initModel.B;
			inv.gm[y*ydim+x]	   = initModel.gm;
			inv.az[y*ydim+x]	   = initModel.az;
			inv.eta0[y*ydim+x]	   = initModel.eta0;
			inv.dopp[y*ydim+x]	   = initModel.dopp;
			inv.aa[y*ydim+x]	   = initModel.aa;
			inv.vlos[y*ydim+x]	   = initModel.vlos; //km/s
			inv.alfa[y*ydim+x]	   = initModel.alfa; //stay light factor
			inv.S0[y*ydim+x]	   = initModel.S0;
			inv.S1[y*ydim+x]	   = initModel.S1;
			inv.nchisqrf[y*ydim+x] = chisqrf;
			
			//exit(-1);
			
		}
	}

	printf("\n Inversion complete.\n");
	// fits file writing operation goes here
	/*
	fclose(fichero);
	
	if(CLASSICAL_ESTIMATES)
		fclose(fichero_estimacioneClasicas);
	*/

	// TODO automatic cmilos_out location!
    //char opath[] = "/scratch/slam/loeschl/dev/sophism/sophism_results/pmi/cmilos_out.fits";
	write_fits(&img, &inv, nombre);

	free(spectro);
	free(lambda);
	free(cuantic);
	free(wlines);

	free_img_struct(&img);
	free_inv_struct(&inv);

	LiberarMemoriaSinteisisDerivadas();
	Liberar_Puntero_Calculos_Compartidos();

	free(G);
		
	return 0;
}


int main_sequential(int argc,char **argv){
	
	double * wlines;
	int nwlines;
	double *lambda;
	int nlambda;
	PRECISION *spectro;
	int ny,i,j;
	Init_Model initModel;
	int err;
	double chisqrf;
	int iter;
	double slight;
	double toplim;
	int miter;
	PRECISION weight[4]={1.,1.,1.,1.};
	int nweight;

	// CONFIGURACION DE PARAMETROS A INVERTIR	
	//INIT_MODEL=[eta0,magnet,vlos,landadopp,aa,gamma,azi,B1,B2,macro,alfa]
	int fix[]={1.,1.,1.,1.,1.,1.,1.,1.,1.,0.,0.}; 
	//----------------------------------------------	
	
	double sigma[NPARMS];
	double vsig;
	double filter;
	double ilambda;
	double noise;
	double *pol;
	double getshi;	
	
	double dat[7]={CUANTIC_NWL,CUANTIC_SLOI,CUANTIC_LLOI,CUANTIC_JLOI,CUANTIC_SUPI,CUANTIC_LUPI,CUANTIC_JUPI};

	char *nombre,*input_iter;
	int Max_iter;

	//milos NLAMBDA MAX_ITER CLASSICAL_ESTIMATES[FWHM DELTA NPOINTS] perfil.txt
	
	if(argc!=5 && argc !=8 ){
		printf("milos: Error en el numero de parametros: %d .\n Pruebe: milos NLAMBDA MAX_ITER CLASSICAL_ESTIMATES [FWHM(in A) DELTA(in A) NPOINTS] perfil.txt\n",argc);
		return -1;
	}	
		
	NLAMBDA = atoi(argv[1]);
	
	input_iter = argv[2];
	Max_iter = atoi(input_iter);
	CLASSICAL_ESTIMATES = atoi(argv[3]);
	
	if(CLASSICAL_ESTIMATES!=0 && CLASSICAL_ESTIMATES != 1){
		printf("milos: Error in CLASSICAL_ESTIMATES parameter. 0 or 1 are valid values. Not accepted: \n",CLASSICAL_ESTIMATES);
		return -1;
	}
	
	if(argc ==5){		
		nombre = argv[4];
	}
	else{	
		INSTRUMENTAL_CONVOLUTION = 1;
		NMUESTRAS_G = atoi(argv[6]);
		FWHM = atof(argv[4]);
		DELTA = atof(argv[5]);
		nombre = argv[7];
	}

	
	//Generamos la gaussiana -> perfil instrumental			
	if(INSTRUMENTAL_CONVOLUTION){
		G=vgauss(FWHM,NMUESTRAS_G,DELTA);
		
		//if you wish to convolution with other instrumental profile you have to declare here and to asign it to "G"
	}

	
	cuantic=create_cuantic(dat);
	Inicializar_Puntero_Calculos_Compartidos();	

	toplim=1e-18;

	CC=PI/180.0;
	CC_2=CC*2;

	filter=0;
	getshi=0;
	nweight=4;

	nwlines=1;
	wlines=(double*) calloc(2,sizeof(double));
	wlines[0]=1;
	wlines[1]= CENTRAL_WL;
	
	vsig=NOISE_SIGMA; //original 0.001
	sigma[0]=vsig;
	sigma[1]=vsig;
	sigma[2]=vsig;
	sigma[3]=vsig;
	pol=NULL;

	noise=NOISE_SIGMA;
	ilambda=ILAMBDA;
	iter=0;
	miter=Max_iter;

	nlambda=NLAMBDA;
	lambda=calloc(nlambda,sizeof(double));
	spectro=calloc(nlambda*4,sizeof(PRECISION));

	FILE *fichero, *fichero_estimacioneClasicas;
// TODO: cfitsio file IO, classical estimates won't work with this for now
	fichero= fopen(nombre,"r");
	if(fichero==NULL){   
		printf("Error de apertura, es posible que el fichero no exista.\n");
		printf("Milos: Error de lectura del fichero. ++++++++++++++++++\n");
		return 1;
	}

	char * buf;
	buf=calloc(strlen(nombre)+15,sizeof(char));
	buf = strcat(buf,nombre);
	buf = strcat(buf,"_CL_ESTIMATES");
	
	if(CLASSICAL_ESTIMATES){
		fichero_estimacioneClasicas= fopen(buf,"w");
		if(fichero_estimacioneClasicas==NULL){   
			printf("Error de apertura ESTIMACIONES CLASICAS, es posible que el fichero no exista.\n");
			printf("Milos: Error de lectura del fichero. ++++++++++++++++++\n");
			return 1;
		}		
	}
	int neje;
	double lin;	
	double iin,qin,uin,vin;
	int rfscanf;
	int contador;	

	int totalIter=0;

	contador=0;

	ReservarMemoriaSinteisisDerivadas(nlambda);

	//initializing weights
	PRECISION *w,*sig;
	weights_init(nlambda,sigma,weight,nweight,&w,&sig,noise);

	c2total=0;
	ctritotal=0;

	int nsub,indaux;
	indaux=0;
		

	do{
		neje=0;
		nsub=0;
		while (neje<NLAMBDA && (rfscanf=fscanf(fichero,"%lf %le %le %le %le",&lin,&iin,&qin,&uin,&vin))!= EOF){
			lambda[nsub]=lin;	
			//printf(" %f \n",lambda[nsub]);
			spectro[nsub]=iin;
			spectro[nsub+NLAMBDA]=qin;
			spectro[nsub+NLAMBDA*2]=uin;
			spectro[nsub+NLAMBDA*3]=vin;
			nsub++;

			neje++;
		}
	
		if(rfscanf!=EOF){  //   && contador==8


		//Initial Model		
	
		initModel.eta0 = INITIAL_MODEL_ETHA0;
		initModel.B = INITIAL_MODEL_B; //200 700
		initModel.gm = INITIAL_MODEL_GM;
		initModel.az = INITIAL_MODEL_AZI;
		initModel.vlos = INITIAL_MODEL_VLOS; //km/s 0
		initModel.mac = 0.0;
		initModel.dopp = INITIAL_MODEL_LAMBDADOPP;
		initModel.aa = INITIAL_MODEL_AA;
		initModel.alfa = 1;							//0.38; //stray light factor
		initModel.S0 = INITIAL_MODEL_S0;
		initModel.S1 = INITIAL_MODEL_S1;

	
		if(CLASSICAL_ESTIMATES){
			estimacionesClasicas(wlines[1],lambda,nlambda,spectro,&initModel);
		
			//OJO CON LOS MENOS !!!

			if(isnan(initModel.B))
				initModel.B = 0;
			if(isnan(initModel.vlos))
				initModel.vlos = 0;
			if(isnan(initModel.gm))
				initModel.gm=0;
			if(isnan(initModel.az))
				initModel.az = 0;
			
			//Escribimos en fichero las estimaciones clásicas
			fprintf(fichero_estimacioneClasicas,"%e %e %e %e\n",initModel.B,initModel.gm,initModel.az,initModel.vlos);
		}	
			
		//inversion	
		lm_mils(cuantic,wlines,nwlines,lambda, nlambda,spectro,nlambda,&initModel,spectra,err,&chisqrf,&iter,slight,toplim,miter,
				weight,nweight,fix,sig,filter,ilambda,noise,pol,getshi,0);

		totalIter+=iter;

		//chisqrf_array[contador]=chisqrf;

		//[contador;iter;B;GM;AZI;etha0;lambdadopp;aa;vlos;S0;S1;final_chisqr];
		printf("%d\n",contador);
		printf("%d\n",iter);
		printf("%f\n",initModel.B);
		printf("%f\n",initModel.gm);
		printf("%f\n",initModel.az);
		printf("%f \n",initModel.eta0);
		printf("%f\n",initModel.dopp);
		printf("%f\n",initModel.aa);
		printf("%f\n",initModel.vlos); //km/s
		//printf("alfa \t:%f\n",initModel.alfa); //stay light factor
		printf("%f\n",initModel.S0);
		printf("%f\n",initModel.S1);
		printf("%.10e\n",chisqrf);
		
		//exit(-1);
		
		}

		contador++;

	}while(rfscanf!=EOF);

	// fits file writing operation goes here
	fclose(fichero);
	
	if(CLASSICAL_ESTIMATES)
		fclose(fichero_estimacioneClasicas);
	

	free(spectro);
	free(lambda);
	free(cuantic);
	free(wlines);

	LiberarMemoriaSinteisisDerivadas();
	Liberar_Puntero_Calculos_Compartidos();

	free(G);
		
	return 0;
}




/*
 * 
 * nwlineas :   numero de lineas espectrales
 * wlines :		lineas spectrales
 * lambda :		wavelength axis in angstrom
			longitud nlambda
 * spectra : IQUV por filas, longitud ny=nlambda
 */

int lm_mils(Cuantic *cuantic,double * wlines,int nwlines,double *lambda,int nlambda,PRECISION *spectro,int nspectro,
		Init_Model *initModel, PRECISION *spectra,int err,double *chisqrf, int *iterOut,
		double slight, double toplim, int miter, PRECISION * weight,int nweight, int * fix,
		PRECISION *sigma, double filter, double ilambda, double noise, double *pol,
		double getshi,int triplete)
{

	int * diag;
	int	iter;
	int i,j,In,*fixed,nfree;
	static PRECISION delta[NTERMS];
	double max[3],aux;
	int repite,pillado,nw,nsig;
	double *landa_store,flambda;
	static PRECISION beta[NTERMS],alpha[NTERMS*NTERMS];
	double chisqr,ochisqr;
	int nspectra,nd_spectra,clanda,ind;
	Init_Model model;

	//Genera aleatoriamente los componentes del vector
	tiempo(semi);					//semilla para  generar los valores de la lista de forma aleatoria con srand
	srand((char)semi);

	iter=0;


	//nterms= 11; //numero de elementomodel->gms de initmodel
	nfree=CalculaNfree(spectro,nspectro);
	//printf("\n nfree! %d:\n",nfree);
	//exit(-1);


	if(nfree==0){
		return -1; //'NOT ENOUGH POINTS'
	}

	flambda=ilambda;

	if(fix==NULL){
		fixed=calloc(NTERMS,sizeof(double));
		for(i=0;i<NTERMS;i++){
			fixed[i]=1;
		}
	}
	else{
		fixed=fix;
	}

	clanda=0;
	iter=0;
//	landa_store=calloc(miter+1,sizeof(double));
	repite=1;
	pillado=0;

	static PRECISION covar[NTERMS*NTERMS];
	static PRECISION betad[NTERMS];

	PRECISION chisqr_mem;
	int repite_chisqr=0;


	/**************************************************************************/
	mil_sinrf(cuantic,initModel,wlines,nwlines,lambda,nlambda,spectra,AH,slight,triplete,filter);


/*	printf(" Acceso spectra \n");
	int kk;
	for(kk=0;kk<24;kk++)
		printf(" withn MIL --------------- %d .... %e \n",kk,spectra[kk]);

	printf(" Acceso G \n");
	for(kk=0;kk<NMUESTRAS_G;kk++)
		printf(" G - %d .... %e \n",kk,G[kk]);
	*/

	//convolucionamos los perfiles IQUV (spectra)
	spectral_synthesis_convolution();

	/*printf(" Acceso spectra \n");
	for(kk=0;kk<24;kk++)
		printf(" withn MIL --------------- %d .... %e \n",kk,spectra[kk]);
	*/
	me_der(cuantic,initModel,wlines,nwlines,lambda,nlambda,d_spectra,AH,slight,triplete,filter);

	/*
	printf(" Acceso d_spectra \n");

	for(kk=0;kk<24;kk++)
		printf(" withn MIL --------------- %d .... %e \n",kk,d_spectra[kk]);
	printf(" return \n");
	*/
	//convolucionamos las funciones respuesta ( d_spectra )
	response_functions_convolution();

	//FijaACeroDerivadasNoNecesarias(d_spectra,fixed,nlambda);

	covarm(weight,sigma,nsig,spectro,nlambda,spectra,d_spectra,beta,alpha);

	for(i=0;i<NTERMS;i++)
		betad[i]=beta[i];

	for(i=0;i<NTERMS*NTERMS;i++)
		covar[i]=alpha[i];

	/**************************************************************************/

	ochisqr=fchisqr(spectra,nspectro,spectro,weight,sigma,nfree);


	model=*initModel;
	do{
		chisqr_mem=(PRECISION)ochisqr;

		/**************************************************************************/
		for(i=0;i<NTERMS;i++){
			ind=i*(NTERMS+1);
			covar[ind]=alpha[ind]*(1.0+flambda);
		}


		mil_svd(covar,betad,delta);

		AplicaDelta(initModel,delta,fixed,&model);

		check(&model);

		/**************************************************************************/

		mil_sinrf(cuantic,&model,wlines,nwlines,lambda,nlambda,spectra,AH,slight,triplete,filter);


		//convolucionamos los perfiles IQUV (spectra)
		spectral_synthesis_convolution();


		chisqr=fchisqr(spectra,nspectro,spectro,weight,sigma,nfree);

		/**************************************************************************/
		if(chisqr-ochisqr < 0){

			flambda=flambda/10.0;

			*initModel=model;


			// printf("iteration=%d , chisqr = %f CONVERGE	- lambda= %e \n",iter,chisqr,flambda);


			me_der(cuantic,initModel,wlines,nwlines,lambda,nlambda,d_spectra,AH,slight,triplete,filter);

			//convolucionamos las funciones respuesta ( d_spectra )
			response_functions_convolution();

			//FijaACeroDerivadasNoNecesarias(d_spectra,fixed,nlambda);

			covarm(weight,sigma,nsig,spectro,nlambda,spectra,d_spectra,beta,alpha);
			for(i=0;i<NTERMS;i++)
				betad[i]=beta[i];

			for(i=0;i<NTERMS*NTERMS;i++)
				covar[i]=alpha[i];

			ochisqr=chisqr;
		}
		else{
			flambda=flambda*10;//10;

			// printf("iteration=%d , chisqr = %f NOT CONVERGE	- lambda= %e \n",iter,ochisqr,flambda);

		}

		/**************************************************************************/
		iter++;

/*
		printf("\n-----------------------\n");
		printf("%d\n",iter);
		printf("%f\n",initModel->B);
		printf("%f\n",initModel->gm);
		printf("%f\n",initModel->az);
		printf("%f \n",initModel->eta0);
		printf("%f\n",initModel->dopp);
		printf("%f\n",initModel->aa);
		printf("%f\n",initModel->vlos); //km/s
		//printf("alfa \t:%f\n",initModel.alfa); //stay light factor
		printf("%f\n",initModel->S0);
		printf("%f\n",initModel->S1);
		printf("%.10e\n",ochisqr);
*/

	}while(iter<=miter); // && !clanda);

	*iterOut=iter;

	*chisqrf=ochisqr;

	//For debugging -> flambda en lugar de AA
	//initModel->aa = flambda;


	if(fix==NULL)
		free(fixed);


	return 1;
}

int CalculaNfree(PRECISION *spectro,int nspectro){
	int nfree,i,j;
	nfree=0;


	// for(j=0;j<4*nspectro;j++){
		// if(spectro[j]!=0.0){
			// nfree++;
		// }
	// }
	// nfree=nfree-NTERMS;//NTERMS;


	nfree = (nspectro*NPARMS) - NTERMS;


	return nfree;
}


/*
*
*
* Cálculo de las estimaciones clásicas.
*
*
* lambda_0 :  centro de la línea
* lambda :    vector de muestras
* nlambda :   numero de muesras
* spectro :   vector [I,Q,U,V]
* initModel:  Modelo de atmosfera a ser modificado
*
*
*
* @Author: Juan Pedro Cobos Carrascosa (IAA-CSIC)
*		   jpedro@iaa.es
* @Date:  Nov. 2011
*
*/
void estimacionesClasicas(PRECISION lambda_0,double *lambda,int nlambda,PRECISION *spectro,Init_Model *initModel){

	PRECISION x,y,aux,LM_lambda_plus,LM_lambda_minus,Blos,beta_B,Ic,Vlos;
	PRECISION *spectroI,*spectroQ,*spectroU,*spectroV;
	PRECISION L,m,gamma, gamma_rad,tan_gamma,maxV,minV,C,maxWh,minWh;
	int i,j;


	//Es necesario crear un lambda en FLOAT para probar como se hace en la FPGA
	PRECISION *lambda_aux;
	lambda_aux= (PRECISION*) calloc(nlambda,sizeof(PRECISION));
	PRECISION lambda0,lambda1,lambda2,lambda3,lambda4;

	lambda0 = 6.1732012e+3 + 0; // RTE_WL_0
	lambda1 = lambda0 + 0.070000000; //RTE_WL_STEP
	lambda2 = lambda1 + 0.070000000;
	lambda3 = lambda2 + 0.070000000;
	lambda4 = lambda3 + 0.070000000;

	lambda_aux[0]=lambda0;
	lambda_aux[1]=lambda1;
	lambda_aux[2]=lambda2;
	lambda_aux[3]=lambda3;
	lambda_aux[4]=lambda4;

	//Sino queremos usar el lambda de la FPGA
	for(i=0;i<nlambda-1;i++){
		lambda_aux[i] = (PRECISION)lambda[i];
	}


	spectroI=spectro;
	spectroQ=spectro+nlambda;
	spectroU=spectro+nlambda*2;
	spectroV=spectro+nlambda*3;

	Ic= spectro[nlambda-1]; // Continuo ultimo valor de I
	Ic= spectro[0]; // Continuo ultimo valor de I


	x=0;
	y=0;
	for(i=0;i<nlambda-1;i++){
		aux = ( Ic - (spectroI[i]+ spectroV[i]));
		x= x +  aux * (lambda_aux[i]-lambda_0);
		y = y + aux;
	}

	//Para evitar nan
	if(fabs(y)>1e-15)
		LM_lambda_plus	= x / y;
	else
		LM_lambda_plus = 0;
	// LM_lambda_plus	= x / y;

	x=0;
	y=0;
	for(i=0;i<nlambda-1;i++){
		aux = ( Ic - (spectroI[i] - spectroV[i]));
		x= x +  aux * (lambda_aux[i]-lambda_0);
		y = y + aux;
	}

	if(fabs(y)>1e-15)
		LM_lambda_minus	= x / y;
	else
		LM_lambda_minus = 0;
	// LM_lambda_minus	= x / y;

	C = (CTE4_6_13 * lambda_0 * lambda_0 * cuantic->GEFF);
	beta_B = 1 / C;

	// printf("beta_B %20.12e \n",beta_B/2);
	// printf("beta_v %20.12e \n",( VLIGHT / (lambda_0))/2);
	// printf("cuantic->GEFF %f \n",cuantic->GEFF);
	// exit(-1);

	Blos = beta_B * ((LM_lambda_plus - LM_lambda_minus)/2);
	Vlos = ( VLIGHT / (lambda_0)) * ((LM_lambda_plus + LM_lambda_minus)/2);


	//------------------------------------------------------------------------------------------------------------
	// //Para test del modo "non-polarimetric"	--> Calculo de Vlos solo con I

	// x=0;
	// y=0;
	// for(i=0;i<nlambda-1;i++){
	// 	aux = ( Ic - (spectroI[i]));
	// 	x= x +  aux * (lambda_aux[i]-lambda_0);
	// 	y = y + aux;
	// }
  //
	// //Para evitar nan
	// LM_lambda_plus	= x / y;
	// // if(fabs(y)>1e-12)
	// 	// LM_lambda_plus	= x / y;
	// // else
	// 	// LM_lambda_plus = 0;
  //
	// Vlos = ( VLIGHT / (lambda_0)) * ((LM_lambda_plus));

	//------------------------------------------------------------------------------------------------------------


	//------------------------------------------------------------------------------------------------------------
	// //Para test del modo "Longitudinal"	--> Calculo de Blos y Vlos solo con I+V y I-V
	// //Los datos vienen en la forma de siempre salvo que spectroI contiene I+V y spectroV contiene I-V
  //
	// //Para usar los mismos perfiles de siempre es necesario tunearlos y convertirlos a I+V y I-V
	// for(i=0;i<nlambda;i++){
	// 	aux = spectroI[i];
	// 	spectroI[i] = spectroI[i] + spectroV[i];
	// 	spectroV[i] = aux - spectroV[i];
	// }
  //
  //
	// //Calculo LONGITUDINAL
	// Ic= spectroI[nlambda-1]; // Continuo ultimo valor de I
  //
  //
	// x=0;
	// y=0;
	// for(i=0;i<nlambda-1;i++){
	// 	aux = ( Ic - (spectroI[i]));
	// 	x= x +  aux * ((float)lambda_aux[i]-(float)lambda_0);
	// 	y = y + aux;
	// }
  //
	// // // para probar si el orden de las operaciones afecta al redondeo
	// // PRECISION xb,dist0,dist1,dist2,dist3,dist4,aux0,aux1,aux2,aux3,aux4;
	// // PRECISION a3,a6,a9,a12,a15,a16,a17,a18,a19;
  // //
  // //
	// // lambda0 = 6.1732012e+3 + 0;
	// // lambda1 = lambda0 + 0.070000000;
	// // lambda2 = lambda1 + 0.070000000;
	// // lambda3 = lambda2 + 0.070000000;
	// // lambda4 = lambda3 + 0.070000000;
  // //
  // //
	// // dist0 = (lambda0-(float)lambda_0);
	// // dist1 = (lambda1-(float)lambda_0);
	// // dist2 = (lambda2-(float)lambda_0);
	// // dist3 = (lambda3-(float)lambda_0);
	// // dist4 = (lambda4-(float)lambda_0);
  // //
	// // aux0 = Ic - spectroI[0];
	// // aux1 = Ic - spectroI[1];
	// // aux2 = Ic - spectroI[2];
	// // aux3 = Ic - spectroI[3];
	// // aux4 = Ic - spectroI[4];
  // //
	// // a3 = aux0*dist0;
	// // a6 = aux1*dist1;
	// // a9 = aux2*dist2;
	// // a12 = aux3*dist3;
	// // a15 = aux4*dist4;
  // //
	// // a16= a3+a6;
	// // a17= a16+a9;
	// // a18= a17+a12;
	// // a19= a18+a15;
  // //
	// // printf("lambda_0 %20.7e \n",(float)lambda_0);
	// // printf("a19 B %20.7e \n",(float)a19);
	// // // fin prueba redondeo
  //
	// LM_lambda_plus	= x / y;
  //
	// // printf("a19 %20.12e \n",(float)x);
	// // printf("LM_lambda_plus %20.12e \n",(float)LM_lambda_plus);
  //
  //
	// // //Para evitar nan
	// // if(fabs(y)>1e-12)
	// 	// LM_lambda_plus	= x / y;
	// // else
	// 	// LM_lambda_plus = 0;
  //
  //
	// x=0;
	// y=0;
	// for(i=0;i<nlambda-1;i++){
	// 	aux = ( Ic - (spectroV[i]));
	// 	x= x +  aux * (lambda_aux[i]-lambda_0);
	// 	y = y + aux;
	// }
  //
  //
	// LM_lambda_minus	= x / y;
  //
	// // printf("b19 %20.12e \n",(float)x);
	// // printf("LM_lambda_minus %20.12e \n",(float)LM_lambda_minus);
  //
	// // //Para evitar nan
	// // if(fabs(y)>1e-12)
	// 	// LM_lambda_minus	= x / y;
	// // else
	// 	// LM_lambda_minus = 0;
  //
  //
	// C = ((float)CTE4_6_13 * (float)lambda_0 * (float)lambda_0 * (float)cuantic->GEFF);
	// beta_B = 1 / C;
  //
	// // printf("beta_B %20.12e \n",(float)beta_B/2);
	// // printf("beta_B (no float cast) %20.12e \n",((1/(CTE4_6_13 * lambda_0 * lambda_0 * cuantic->GEFF))/2));
	// // printf("beta_v %20.12e \n",(float)(( (float)VLIGHT / ((float)lambda_0))/2));
	// // printf("cuantic->GEFF %f \n",cuantic->GEFF);
	// // exit(-1);
  //
	// Blos = beta_B * ((LM_lambda_plus - LM_lambda_minus)/2);
	// Vlos = ( VLIGHT / (lambda_0)) * ((LM_lambda_plus + LM_lambda_minus)/2);
  //
	// // printf("spectroI_0 %20.7e \n",(float)spectroI[0]);
	// // printf("spectroI_1 %20.7e \n",(float)spectroI[1]);
	// // printf("spectroI_2 %20.7e \n",(float)spectroI[2]);
	// // printf("spectroI_3 %20.7e \n",(float)spectroI[3]);
	// // printf("spectroI_4 %20.7e \n",(float)spectroI[4]);
	// // printf("spectroI_5 %20.7e \n",(float)spectroI[5]);
	// // printf("Blos %20.12e \n",(float)Blos);
	// // printf("Vlos %20.12e \n",(float)Vlos);
  // //
	// // exit(-1);
	//------------------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------------------
	// //Para probar fórmulación propuesta por D. Orozco (Junio 2017)
	//La formula es la 2.7 que proviene del paper:
	// Diagnostics for spectropolarimetry and magnetography by Jose Carlos del Toro Iniesta and Valent´ýn Mart´ýnez Pillet
	//el 0.08 Es la anchura de la línea en lugar de la resuloción del etalón.

	//Vlos = ( 2*(VLIGHT)*0.08 / (PI*lambda_0)) * atan((spectroI[0]+spectroI[1]-spectroI[3]-spectroI[4])/(spectroI[0]-spectroI[1]-spectroI[3]+spectroI[4]));

	//------------------------------------------------------------------------------------------------------------


	Blos=Blos*1 ; //factor de correción x campo debil
	Vlos = Vlos * 1 ; //factor de correción ...

	//inclinacion
	x = 0;
	y = 0;
	for(i=0;i<nlambda-1;i++){
		L = fabs( sqrtf( spectroQ[i]*spectroQ[i] + spectroU[i]*spectroU[i] ));
		m = fabs( (4 * (lambda_aux[i]-lambda_0) * L ));// / (3*C*Blos) ); //2*3*C*Blos mod abril 2016 (en test!)

		x = x + fabs(spectroV[i]) * m;
		y = y + fabs(spectroV[i]) * fabs(spectroV[i]);

//		printf("L %f \n",L);
//		printf("m : %f \n",m);
	}

	y = y * fabs((3*C*Blos));

	//for debuging
	// x = fabs(x) > 1e-7 ? x : 0;
	// y = fabs(y) > 1e-7 ? y : 0;

	tan_gamma = fabs(sqrtf(x/y));

	// tan_gamma = fabs(tan_gamma) > 1e-7 ? tan_gamma : 0;

	gamma_rad = atan(tan_gamma); //gamma en radianes

	// if(sqrt(y)<1e-12)
		// gamma_rad = PI/2;
	// else
		// gamma_rad = atan2(sqrt(x),sqrt(y)); //gamma en radianes

	//gamma_rad = atan(sqrtf(x),y)); //gamma en radianes

	// gamma_rad = fabs(gamma_rad) > 1e-7 ? gamma_rad : 0;

	gamma = gamma_rad * (180/ PI); //gamma en grados

	//correccion
	//utilizamos el signo de Blos para ver corregir el cuadrante
	PRECISION gamma_out = gamma;

    if (Blos<0)
        gamma = (180)-gamma;




	//azimuth

	PRECISION tan2phi,phi;
	int muestra;

	if(nlambda==6)
		muestra = CLASSICAL_ESTIMATES_SAMPLE_REF;
	else
		muestra = nlambda*0.75;


	tan2phi=spectroU[muestra]/spectroQ[muestra];

	// printf("tan2phi : %f \n",tan2phi);
	// printf("%.10e \n",spectroU[muestra]);
	// printf("%.10e \n",spectroQ[muestra]);


	phi= (atan(tan2phi)*180/PI) / 2;  //atan con paso a grados

	//printf("atan : %f \n",phi*2);

	// printf("%.10e \n",atan(tan2phi));

	if(spectroU[muestra] > 0 && spectroQ[muestra] > 0 )
		phi=phi;
	else
	if (spectroU[muestra] < 0 && spectroQ[muestra] > 0 )
		phi=phi + 180;
	else
	if (spectroU[muestra] < 0 && spectroQ[muestra] < 0 )
		phi=phi + 90;
	else
	if (spectroU[muestra] > 0 && spectroQ[muestra]< 0 )
			phi=phi + 90;

	// printf("%.10e \n",phi);

	//printf("Blos : %f \n",Blos);
	//printf("vlos : %f \n",Vlos);
	//printf("gamma : %f \n",gamma);
	//printf("phi : %f \n",phi);


	PRECISION B_aux;

	B_aux = fabs(Blos/cos(gamma_rad)) * 2; // 2 factor de corrección

	//Vlos = Vlos * 1.5;
	if(Vlos < (-20))
		Vlos= -20;
	if(Vlos >(20))
		Vlos=(20);

	// if(phi< 0)
		// phi = 180 + (phi);
	// if(phi > 180){
		// phi = phi -180.0;
	// }

	// printf("%.10e \n",phi);

	initModel->B = (B_aux>4000?4000:B_aux);
	initModel->vlos=Vlos;//(Vlos*1.5);//1.5;
	initModel->gm=gamma;
	initModel->az=phi;
	initModel->S0= Blos;


	//Liberar memoria del vector de lambda auxiliar
	free(lambda_aux);

}

void FijaACeroDerivadasNoNecesarias(PRECISION * d_spectra,int *fixed,int nlambda){

	int In,j,i;
	for(In=0;In<NTERMS;In++)
		if(fixed[In]==0)
			for(j=0;j<4;j++)
				for(i=0;i<nlambda;i++)
					d_spectra[i+nlambda*In+j*nlambda*NTERMS]=0;
}

void AplicaDelta(Init_Model *model,PRECISION * delta,int * fixed,Init_Model *modelout){

	//INIT_MODEL=[eta0,magnet,vlos,landadopp,aa,gamma,azi,B1,B2,macro,alfa]

	if(fixed[0]){
		modelout->eta0=model->eta0-delta[0]; // 0
	}
	if(fixed[1]){
		if(delta[1]< -800) //300
			delta[1]=-800;
		else
			if(delta[1] >800)
				delta[1]=800;
		modelout->B=model->B-delta[1];//magnetic field
	}
	if(fixed[2]){

		 if(delta[2]>2)
			 delta[2] = 2;

		 if(delta[2]<-2)
			delta[2] = -2;

		modelout->vlos=model->vlos-delta[2];
	}

	if(fixed[3]){

		if(delta[3]>1e-2)
			delta[3] = 1e-2;
		else
			if(delta[3]<-1e-2)
				delta[3] = -1e-2;

		modelout->dopp=model->dopp-delta[3];
	}

	if(fixed[4])
		modelout->aa=model->aa-delta[4];

	if(fixed[5]){
		if(delta[5]< -15) //15
			delta[5]=-15;
		else
			if(delta[5] > 15)
				delta[5]=15;

		modelout->gm=model->gm-delta[5]; //5
	}
	if(fixed[6]){
		if(delta[6]< -15)
			delta[6]=-15;
		else
			if(delta[6] > 15)
				delta[6]= 15;

		modelout->az=model->az-delta[6];
	}
	if(fixed[7])
		modelout->S0=model->S0-delta[7];
	if(fixed[8])
		modelout->S1=model->S1-delta[8];
	if(fixed[9])
		modelout->mac=model->mac-delta[9]; //9
	if(fixed[10])
		modelout->alfa=model->alfa-delta[10];
}

/*
	Tamaño de H es 	 NTERMS x NTERMS
	Tamaño de beta es 1xNTERMS

	return en delta tam 1xNTERMS
*/

int mil_svd(PRECISION *h,PRECISION *beta,PRECISION *delta){

	double epsilon,top;
	static PRECISION v2[TAMANIO_SVD][TAMANIO_SVD],w2[TAMANIO_SVD],v[NTERMS*NTERMS],w[NTERMS];
	static PRECISION h1[NTERMS*NTERMS],h_svd[TAMANIO_SVD*TAMANIO_SVD];
	static PRECISION aux[NTERMS*NTERMS];
	int i,j;
//	static double aux2[NTERMS*NTERMS];
	static	PRECISION aux2[NTERMS];
	int aux_nf,aux_nc;
	PRECISION factor,maximo,minimo;
	int posi,posj;

	epsilon= 1e-12;
	top=1.0;

	factor=0;
	maximo=0;
	minimo=1000000000;


	/**/
	for(j=0;j<NTERMS*NTERMS;j++){
		h1[j]=h[j];
	}

	// //Para imprimir matrices
	// int i1,j1;
	// printf("#################################### h1 \n");
	// for(j1=0;j1<NTERMS;j1++){
		// for(i1=0;i1<NTERMS;i1++){
			// printf("%20.12e ,",h1[j1*NTERMS + i1]);
		// }
		// printf("\n");
	// }
	//exit(-1);


	if(USE_SVDCMP){

		svdcmp(h1,NTERMS,NTERMS,w,v);


	}
	else{
		//printf(" NORMALIZACION y CORDIC######################################\n");
		//	NORMALIZACION
		for(j=0;j<NTERMS*NTERMS;j++){
				if(fabs(h[j])>maximo){
					maximo=fabs(h[j]);
				}
			}

		factor=maximo;

		//printf("maximo : %.12e \n",maximo);
		//exit(-1);


		if(!NORMALIZATION_SVD)
			factor = 1;

		for(j=0;j<NTERMS*NTERMS;j++){
			h1[j]=h[j]/(factor );
		}


		for(i=0;i<TAMANIO_SVD-1;i++){
			for(j=0;j<TAMANIO_SVD;j++){
				if(j<NTERMS)
					h_svd[i*TAMANIO_SVD+j]=h1[i*NTERMS+j];
				else
					h_svd[i*TAMANIO_SVD+j]=0;
			}
		}

		for(j=0;j<TAMANIO_SVD;j++){
			h_svd[(TAMANIO_SVD-1)*TAMANIO_SVD+j]=0;
		}

		svdcordic(h_svd,TAMANIO_SVD,TAMANIO_SVD,w2,v2,NUM_ITER_SVD_CORDIC);

		for(i=0;i<TAMANIO_SVD-1;i++){
			for(j=0;j<TAMANIO_SVD-1;j++){
				v[i*NTERMS+j]=v2[i][j];
			}
		}

		for(j=0;j<TAMANIO_SVD-1;j++){
			w[j]=w2[j]*factor;
		}

	}

	static PRECISION vaux[NTERMS*NTERMS],waux[NTERMS];

	for(j=0;j<NTERMS*NTERMS;j++){
			vaux[j]=v[j];//*factor;
	}

	for(j=0;j<NTERMS;j++){
			waux[j]=w[j];//*factor;
	}

	// //Para imprimir matrices AUTOVAL y AUTOVEC
	// int i1,j1;
	// printf("#################################### w \n");
	// for(j1=0;j1<NTERMS;j1++){
		// printf("%20.12e ,",waux[j1]);
	// }
	// printf("\n");
	// printf("#################################### v \n");
	// for(j1=0;j1<NTERMS;j1++){
		// for(i1=0;i1<NTERMS;i1++){
			// printf("%20.12e ,",vaux[j1*NTERMS + i1]);
		// }
		// printf("\n");
	// }
	// exit(-1);

	multmatrix(beta,1,NTERMS,vaux,NTERMS,NTERMS,aux2,&aux_nf,&aux_nc);

	for(i=0;i<NTERMS;i++){
		aux2[i]= aux2[i]*((fabs(waux[i]) > epsilon) ? (1/waux[i]): 0.0);//((waux[i]>0)?(1/epsilon):(-1/epsilon))); //(1/waux[i]) : 0);//
//		aux2[i]= aux2[i]*((fabs(waux[i]) > epsilon) ? (1/waux[i]): (1/epsilon));//((waux[i]>0)?(1/epsilon):(-1/epsilon))); //(1/waux[i]) : 0);//
	}

	multmatrix(vaux,NTERMS,NTERMS,aux2,NTERMS,1,delta,&aux_nf,&aux_nc);

/*
	printf("\n");
	printf("#################################### delta \n");
	int j1;
	for(j1=0;j1<NTERMS;j1++){
		printf("%20.12e ,",delta[j1]);
	}
	// exit(-1);
*/
	return 1;

}



void weights_init(int nlambda,double *sigma,PRECISION *weight,int nweight,PRECISION **wOut,PRECISION **sigOut,double noise)
{
	int i,j;
	PRECISION *w,*sig;


	sig=calloc(4,sizeof(PRECISION));
	if(sigma==NULL){
		for(i=0;i<4;i++)
			sig[i]=	noise* noise;
	}
	else{

		for(i=0;i<4;i++)
			sig[i]=	(*sigma);// * (*sigma);
	}

	*wOut=w;
	*sigOut=sig;

}


int check(Init_Model *model){

	double offset=0;
	double inter;

	//Inclination
	/*	if(model->gm < 0)
		model->gm = -(model->gm);
	if(model->gm > 180)
		model->gm =180-(((int)floor(model->gm) % 180)+(model->gm-floor(model->gm)));//180-((int)model->gm % 180);*/

	//Magnetic field
	if(model->B < 0){
		//model->B = 190;
		model->B = -(model->B);
		model->gm = 180.0 -(model->gm);
	}
	if(model->B > 5000)
		model->B= 5000;

	//Inclination
	if(model->gm < 0)
		model->gm = -(model->gm);
	if(model->gm > 180){
		model->gm = 360.0 - model->gm;
		// model->gm = 179; //360.0 - model->gm;
	}

	//azimuth
	if(model->az < 0)
		model->az= 180 + (model->az); //model->az= 180 + (model->az);
	if(model->az > 180){
		model->az =model->az -180.0;
		// model->az = 179.0;
	}

	//RANGOS
	//Eta0
	if(model->eta0 < 0.1)
		model->eta0=0.1;

	// if(model->eta0 >8)
			// model->eta0=8;
	if(model->eta0 >2500)  //idl 2500
			model->eta0=2500;

	//velocity
	if(model->vlos < (-20)) //20
		model->vlos= (-20);
	if(model->vlos >20)
		model->vlos=20;

	//doppler width ;Do NOT CHANGE THIS
	if(model->dopp < 0.0001)
		model->dopp = 0.0001;

	if(model->dopp > 1.6)  // idl 0.6
		model->dopp = 1.6;


	if(model->aa < 0.0001)  // idl 1e-4
		model->aa = 0.0001;
	if(model->aa > 10)            //10
		model->aa = 10;

	//S0
	if(model->S0 < 0.0001)
		model->S0 = 0.0001;
	if(model->S0 > 1.500)
		model->S0 = 1.500;

	//S1
	if(model->S1 < 0.0001)
		model->S1 = 0.0001;
	if(model->S1 > 2.000)
		model->S1 = 2.000;

	//macroturbulence
	if(model->mac < 0)
		model->mac = 0;
	if(model->mac > 4)
		model->mac = 4;

	//filling factor
/*	if(model->S1 < 0)
		model->S1 = 0;
	if(model->S1 > 1)
		model->S1 = 1;*/

	return 1;
}

void spectral_synthesis_convolution(){

	int i;
	int nlambda=NLAMBDA;

	//convolucionamos los perfiles IQUV (spectra)
	if(INSTRUMENTAL_CONVOLUTION){

		PRECISION Ic;


		if(!INSTRUMENTAL_CONVOLUTION_INTERPOLACION){
			//convolucion de I
			Ic=spectra[nlambda-1];

			for(i=0;i<nlambda-1;i++)
				spectra[i]=Ic-spectra[i];



			direct_convolution(spectra,nlambda-1,G,NMUESTRAS_G,1);  //no convolucionamos el ultimo valor Ic


			for(i=0;i<nlambda-1;i++)
				spectra[i]=Ic-spectra[i];

			//convolucion QUV
			for(i=1;i<NPARMS;i++)
				direct_convolution(spectra+nlambda*i,nlambda-1,G,NMUESTRAS_G,1);  //no convolucionamos el ultimo valor
		}
		else{
			if(NLAMBDA == 6){

				//convolucion de I
				Ic=spectra[nlambda-1];

				for(i=0;i<nlambda-1;i++)
					spectra[i]=Ic-spectra[i];

				PRECISION *spectra_aux;
				spectra_aux =  (PRECISION*) calloc(nlambda*2-2,sizeof(PRECISION));

				int j=0;
				for(i=0,j=0;i<nlambda*2-2;i=i+2,j++)
					spectra_aux[i]=spectra[j];

				for(i=1,j=0;i<nlambda*2-2;i=i+2,j++)
					spectra_aux[i]=(spectra[j]+spectra[j+1])/2;


				// printf("spectraI_aux=[");
				// for(i=0;i<nlambda*2-2;i++){
					// printf("%f",Ic-spectra_aux[i]);
					// if(i<nlambda*2-2-1)
						// printf(", ");
				// }
				// printf("];\n");

				direct_convolution(spectra_aux,nlambda*2-2-1,G,NMUESTRAS_G,1);  //no convolucionamos el ultimo valor Ic

				// printf("spectraI_aux_conv=[");
				// for(i=0;i<nlambda*2-2;i++){
					// printf("%f",Ic-spectra_aux[i]);
					// if(i<nlambda*2-2-1)
						// printf(", ");
				// }
				// printf("];\n");


				for(i=0,j=0;i<nlambda*2-2;i=i+2,j++)
					spectra[j]=spectra_aux[i];

				for(i=0;i<nlambda-1;i++)
					spectra[i]=Ic-spectra[i];

				free(spectra_aux);

				//convolucion QUV
				int k;
				for(k=1;k<NPARMS;k++){

					PRECISION *spectra_aux;
					spectra_aux =  (PRECISION*) calloc(nlambda*2-2,sizeof(PRECISION));

					int j=0;
					for(i=0,j=0;i<nlambda*2-2;i=i+2,j++)
						spectra_aux[i]=spectra[j+nlambda*k];

					for(i=1,j=0;i<nlambda*2-2;i=i+2,j++)
						spectra_aux[i]=(spectra[j+nlambda*k]+spectra[j+1+nlambda*k])/2;

					direct_convolution(spectra_aux,nlambda*2-2-1,G,NMUESTRAS_G,1);  //no convolucionamos el ultimo valor Ic

					for(i=0,j=0;i<nlambda*2-2;i=i+2,j++)
						spectra[j+nlambda*k]=spectra_aux[i];

					free(spectra_aux);
				}
			}
		}
	}
}



void response_functions_convolution(){

	int i,j;
	int nlambda=NLAMBDA;

	//convolucionamos las funciones respuesta ( d_spectra )
	if(INSTRUMENTAL_CONVOLUTION){
		if(!INSTRUMENTAL_CONVOLUTION_INTERPOLACION){


			for(j=0;j<NPARMS;j++){
				for(i=0;i<NTERMS;i++){
					if(i!=7) //no convolucionamos S0
						direct_convolution(d_spectra+nlambda*i+nlambda*NTERMS*j,nlambda-1,G,NMUESTRAS_G,1);  //no convolucionamos el ultimo valor
				}
			}


		}
		else{

			int k,m;
			for(k=0;k<NPARMS;k++){
				for(m=0;m<NTERMS;m++){

					PRECISION *spectra_aux;
					spectra_aux =  (PRECISION*) calloc(nlambda*2-2,sizeof(PRECISION));

					int j=0;
					for(i=0,j=0;i<nlambda*2-2;i=i+2,j++)
						spectra_aux[i]=d_spectra[j+nlambda*m+nlambda*NTERMS*k];

					for(i=1,j=0;i<nlambda*2-2;i=i+2,j++)
						spectra_aux[i]=(d_spectra[j+nlambda*m+nlambda*NTERMS*k]+d_spectra[j+nlambda*m+nlambda*NTERMS*k])/2;

					direct_convolution(spectra_aux,nlambda*2-2-1,G,NMUESTRAS_G,1);  //no convolucionamos el ultimo valor Ic

					for(i=0,j=0;i<nlambda*2-2;i=i+2,j++)
						d_spectra[j+nlambda*m+nlambda*NTERMS*k]=spectra_aux[i];

					free(spectra_aux);
				}
			}
		}
	}

}
