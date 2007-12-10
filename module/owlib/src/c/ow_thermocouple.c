/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
    email: palfille@earthlink.net
    Released under the GPL
    See the header file: ow.h for full attribution
    1wire/iButton system from Dallas Semiconductor
*/


#include <config.h>
#include "owfs_config.h"
#include "ow_thermocouple.h"

#if OW_THERMOCOUPLE

/* ------- Structures ----------- */

// Note: highest order polynomial coefficient comes first
struct thermocouple_data {
    _FLOAT mV_low, mV_high ;
    _FLOAT temperature_low, temperature_high ;
    struct {
        _FLOAT mV_upper ;
        _FLOAT C[6] ;
    } temperature[4] ;
    _FLOAT mV_coldjunction[6] ;
} ;

struct thermocouple_data Thermocouple_data[e_type_last] = {
	{ // Type_B
		-0.003,13.82, // mV limits
		0,1820, // temperature limits
		{ // mV -> temperature in ranges
			{ -0.003, { -2.51221e+27, 2.13758e+24, 7.56308e+22, 1.74246e+20, 1.03265e+17, 0.826153, } , }, // range 0 upper bound and coefficients
			{ 0.012, { -8.4706e+10, 6.47141e+08, 2.78322e+07, -474183, 4758.86, 41.6374, } , }, // range 1 upper bound and coefficients
			{ 0.857, { 2945.94, -7382.24, 7128.12, -3466.34, 1209.28, 62.3706, } , }, // range 2 upper bound and coefficients
			{ 13.82, { 0.00494198, -0.213432, 3.71517, -34.0892, 260.462, 221.702, } , }, // range 3 upper bound and coefficients
		} ,
		{ -6.88408e-13, 2.15015e-10, -2.50994e-08, 7.0516e-06, -0.000269267, 0.000146246, } , // cold junction temperature -> mV
	} ,  // end Type_B

	{ // Type_E
		-9.835,76.373, // mV limits
		-270,1000, // temperature limits
		{ // mV -> temperature in ranges
			{ -9.688, { 3.26265, 138.271, 2343.84, 19858.7, 84111.6, 142368, } , }, // range 0 upper bound and coefficients
			{ -7.733, { 3.26265, 138.271, 2343.84, 19858.7, 84111.6, 142368, } , }, // range 1 upper bound and coefficients
			{ 6.93, { 0.000149705, -0.00165641, 0.0119336, -0.249013, 17.1028, -0.0143392, } , }, // range 2 upper bound and coefficients
			{ 76.373, { 6.43417e-08, -1.81489e-05, 0.00220796, -0.130036, 15.9906, 3.96364, } , }, // range 3 upper bound and coefficients
		} ,
		{ -3.75653e-12, 9.62809e-10, -1.02812e-07, 5.02521e-05, 0.0586085, -0.000287881, } , // cold junction temperature -> mV
	} ,  // end Type_E

	{ // Type_J
		-8.095,69.553, // mV limits
		-210,1200, // temperature limits
		{ // mV -> temperature in ranges
			{ -5.194, { 0.159313, 4.91673, 60.9921, 377.93, 1191.69, 1450.19, } , }, // range 0 upper bound and coefficients
			{ 10.168, { 6.32835e-05, -0.00164631, 0.0216838, -0.238347, 19.8278, 0.00456731, } , }, // range 1 upper bound and coefficients
			{ 42.472, { 2.94912e-06, -0.000385821, 0.017553, -0.358286, 21.3912, -6.15873, } , }, // range 2 upper bound and coefficients
			{ 69.553, { 4.61439e-06, -0.00139572, 0.166449, -9.73731, 294.793, -3051.08, } , }, // range 3 upper bound and coefficients
		} ,
		{ -3.89357e-13, 1.87325e-10, -8.86007e-08, 3.04136e-05, 0.0503854, 9.80857e-06, } , // cold junction temperature -> mV
	} ,  // end Type_J

	{ // Type_K
		-6.458,54.819, // mV limits
		-270,1370, // temperature limits
		{ // mV -> temperature in ranges
			{ -6.351, { 16.5881, 450.901, 4901.84, 26628.6, 72304.4, 78407.2, } , }, // range 0 upper bound and coefficients
			{ -4.865, { 16.5881, 450.901, 4901.84, 26628.6, 72304.4, 78407.2, } , }, // range 1 upper bound and coefficients
			{ 6.981, { 0.00061062, -0.00863106, 0.0728312, -0.405757, 25.3032, -0.0401767, } , }, // range 2 upper bound and coefficients
			{ 54.819, { 6.64007e-07, -0.00011542, 0.00927615, -0.338909, 29.0458, -18.483, } , }, // range 3 upper bound and coefficients
		} ,
		{ -1.11057e-12, 2.31125e-10, -1.22507e-07, 2.63207e-05, 0.0394385, -0.000247425, } , // cold junction temperature -> mV
	} ,  // end Type_K

	{ // Type_N
		-4.345,47.513, // mV limits
		-270,1300, // temperature limits
		{ // mV -> temperature in ranges
			{ -4.336, { 0., 0., 0., 0., 1033.1, 4220.4, } , }, // range 0 upper bound and coefficients
			{ -3.85, { 9667.39, 195822, 1.58634e+06, 6.42413e+06, 1.30051e+07, 1.05287e+07, } , }, // range 1 upper bound and coefficients
			{ 2.48, { 0.0255197, -0.0353939, -0.0717555, -0.864733, 38.5024, 0.116269, } , }, // range 2 upper bound and coefficients
			{ 47.513, { 1.93299e-06, -0.000295976, 0.0192512, -0.640613, 36.1391, 4.78696, } , }, // range 3 upper bound and coefficients
		} ,
		{ -1.47913e-12, 1.35741e-10, 3.80869e-08, 1.33212e-05, 0.0260551, -0.0012884, } , // cold junction temperature -> mV
	} ,  // end Type_N

	{ // Type_R
		-0.226,21.003, // mV limits
		-50,1760, // temperature limits
		{ // mV -> temperature in ranges
			{ 0.284, { 539.652, -288.34, 116.45, -91.4751, 189.1, -0.0232359, } , }, // range 0 upper bound and coefficients
			{ 3.171, { 0.313262, -3.48528, 16.2165, -43.8973, 173.888, 1.99571, } , }, // range 1 upper bound and coefficients
			{ 15.348, { 0.000132637, -0.00573544, 0.144438, -3.21251, 113.782, 44.5935, } , }, // range 2 upper bound and coefficients
			{ 21.003, { 0.0121004, -1.07146, 37.9227, -670.36, 5987.36, -20588.8, } , }, // range 3 upper bound and coefficients
		} ,
		{ -2.54783e-16, 2.74148e-11, -2.2995e-08, 1.38489e-05, 0.00528974, 0.000102828, } , // cold junction temperature -> mV
	} ,  // end Type_R

	{ // Type_S
		-0.236,18.609, // mV limits
		-50,1760, // temperature limits
		{ // mV -> temperature in ranges
			{ 0.248, { 72.7836, -147.497, 109.389, -80.6029, 184.799, 0.00485658, } , }, // range 0 upper bound and coefficients
			{ 3.269, { 0.268567, -3.07068, 14.6172, -39.81, 172.739, 1.58909, } , }, // range 1 upper bound and coefficients
			{ 14.263, { 4.55662e-05, 0.0011306, -0.0225231, -1.39047, 113.605, 45.2409, } , }, // range 2 upper bound and coefficients
			{ 18.609, { 0.0462634, -3.71475, 119.221, -1911.17, 15382.2, -48710.7, } , }, // range 3 upper bound and coefficients
		} ,
		{ -3.13954e-13, 1.03e-10, -2.77745e-08, 1.25481e-05, 0.00541035, 1.19149e-05, } , // cold junction temperature -> mV
	} ,  // end Type_S

	{ // Type_T
		-6.258,20.872, // mV limits
		-270,400, // temperature limits
		{ // mV -> temperature in ranges
			{ -6.236, { 0, 0, 479.89, 8507, 50338, 99203, } , }, // range 0 upper bound and coefficients
			{ -5.739, { 0, 0, 479.89, 8507, 50338, 99203, } , }, // range 1 upper bound and coefficients
			{ -1.545, { 0.0589872, 0.884043, 5.49725, 15.4262, 49.2583, 13.1116, } , }, // range 2 upper bound and coefficients
			{ 20.872, { 1.77361e-05, -0.00124183, 0.0374592, -0.723968, 25.8858, -0.0114384, } , }, // range 3 upper bound and coefficients
		} ,
		{ -3.30676e-12, 6.45333e-10, -3.91012e-08, 4.19966e-05, 0.0386665, -0.000360973, } , // cold junction temperature -> mV
	} ,  // end Type_T
} ;

/* ------- Prototypes ----------- */

static _FLOAT Poly4Value( _FLOAT * c, _FLOAT x ) ;
static _FLOAT Temperature_from_mV( _FLOAT mV, enum e_thermocouple_type etype ) ;

_FLOAT Thermocouple_range_low( enum e_thermocouple_type etype )
{
	return Thermocouple_data[etype].temperature_low ;
}

_FLOAT Thermocouple_range_high( enum e_thermocouple_type etype )
{
	return Thermocouple_data[etype].temperature_high ;
}

_FLOAT ThermocoupleTemperature( _FLOAT mV_reading, _FLOAT temperature_coldjunction, enum e_thermocouple_type etype)
{
	_FLOAT mV_coldjunction = Poly4Value( Thermocouple_data[etype].mV_coldjunction, temperature_coldjunction ) ;
	return Temperature_from_mV( mV_reading + mV_coldjunction, etype ) ;
}

// Find right range and calculate the temperature from (corrected) mV
static _FLOAT Temperature_from_mV( _FLOAT mV, enum e_thermocouple_type etype )
{
	if ( mV < Thermocouple_data[etype].temperature[1].mV_upper ) {
		if ( mV < Thermocouple_data[etype].temperature[0].mV_upper ) {
			return Poly4Value( Thermocouple_data[etype].temperature[0].C, mV ) ;
		} else {
			return Poly4Value( Thermocouple_data[etype].temperature[1].C, mV ) ;
		}
	} else {
		if ( mV < Thermocouple_data[etype].temperature[2].mV_upper ) {
			return Poly4Value( Thermocouple_data[etype].temperature[2].C, mV ) ;
		} else {
			return Poly4Value( Thermocouple_data[etype].temperature[3].C, mV ) ;
		}
	}
}

// Calculate 4th order polynomial using Homer's rule
// Of form c0 + c1*x + c2*x^2 + c3*x^3 + c4*x^4
static _FLOAT Poly4Value( _FLOAT * c, _FLOAT x )
{
    return c[5] + x * ( c[4] + x * ( c[3] + x * ( c[2] + x * ( c[1] + c[0] * x ) ) ) ) ;
}

#endif							/* OW_THERMOCOUPLE */

#ifdef THERMOCOUPLE_TEST
#include <stdio.h>

void Thermotest( char type, enum e_thermocouple_type etype )
{
	_FLOAT mV ;
	printf("OWFS_%c = [\n",type) ;
	for ( mV = Thermocouple_data[etype].mV_low ; mV < Thermocouple_data[etype].mV_high ; mV += .1 ) {
		printf("%g , %g \n",mV,Temperature_from_mV( mV, etype)) ;
	}
	printf("] ; \n");
}

int main ( void )
{
	Thermotest('b',e_type_b);
	Thermotest('e',e_type_e);
	Thermotest('j',e_type_j);
	Thermotest('k',e_type_k);
	Thermotest('n',e_type_n);
	Thermotest('r',e_type_r);
	Thermotest('s',e_type_s);
	Thermotest('t',e_type_t);

	return 0 ;
}

#endif /* THERMOCOUPLE_TEST */
