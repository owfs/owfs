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

/* Thermocouple data */

/* Thermocouples based on DS2760 chip
    See Dallas App note:
*/

/* Data from http://srdata.nist.gov/its90/useofdatabase/use_of_database.html */

struct poly {
    FLOAT minf ;
    int   order ;
    FLOAT coef[] ;
} ;

struct polys {
    int n ;
    const struct poly * p[] ;
} ;

struct thermo {
    struct polys * volt ;
    struct polys * temp ;
} ;

/* TYPE K */
/* vX gives V = sum coeff(n)*t^n */
const struct poly vK0 = { -270., 10, {
        0.000000000000E+00, 0.394501280250E-01, 0.236223735980E-04, -0.328589067840E-06,
        -0.499048287770E-08,-0.675090591730E-10, -0.574103274280E-12, -0.310888728940E-14,
        -0.104516093650E-16, -0.198892668780E-19, -0.163226974860E-22,
},} ;
const struct poly vK1 = { 0.,9, {
        -0.176004136860E-01, 0.389212049750E-01, 0.185587700320E-04, -0.994575928740E-07,
        0.318409457190E-09, -0.560728448890E-12, 0.560750590590E-15, -0.320207200030E-18,
        0.971511471520E-22, -0.121047212750E-25,
},} ;
struct polys vK = {2,{&vK0,&vK1,}} ;

/* tX gives T = sum coeff(n) * v^n */
const struct poly tK0 = { -5.891, 8, {
         0.0000000E+00, 2.5173462E+01, -1.1662878E+00, -1.0833638E+00,
        -8.9773540E-01, -3.7342377E-01, -8.6632643E-02, -1.0450598E-02,
        -5.1920577E-04,
},} ;
const struct poly tK1 = { -0.000, 9, {
        0.000000E+00, 2.508355E+01, 7.860106E-02, -2.503131E-01,
        8.315270E-02, -1.228034E-02, 9.804036E-04, -4.413030E-05,
        1.057734E-06, -1.052755E-08,
},} ;
const struct poly tK2 = { 20.644, 6, {
         -1.318058E+02, 4.830222E+01, -1.646031E+00, 5.464731E-02,
        -9.650715E-04, 8.802193E-06, -3.110810E-08,
},} ;
struct polys tK = {3,{ &tK0, &tK1, &tK2, }} ;
struct thermo thermoK = { &vK, &tK, } ;

