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
    // this won't compile with some compilers unless size is set.
    //FLOAT coef[] ;
    FLOAT coef[12] ;
} ;

struct polys {
    int n ;
    // this won't compile with some compilers unless size is set.
    //const struct poly * p[] ;
    const struct poly *p[4] ;
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

/* TYPE N */
/* vX gives V = sum coeff(n)*t^n */
const struct poly vN0 = { -270., 8, {
        0.000000000000E+00, 0.261591059620E-01, 0.109574842280E-04, -0.938411115540E-07,
        -0.464120397590E-10, -0.263033577160E-11, -0.226534380030E-13, -0.760893007910E-16,
        -0.934196678350E-19
},} ;
const struct poly vN1 = { 0.,10, {
        0.000000000000E+00, 0.259293946010E-01, 0.157101418800E-04, 0.438256272370E-07,
        -0.252611697940E-09, 0.643118193390E-12, -0.100634715190E-14, 0.997453389920E-18,
        -0.608632456070E-21, 0.208492293390E-24, -0.306821961510E-28
},} ;
struct polys vN = {2,{&vN0,&vN1,}} ;

/* tX gives T = sum coeff(n) * v^n */
const struct poly tN0 = { -3.990, 9, {
        0.0000000E+00, 3.8436847E+01, 1.1010485E+00, 5.2229312E+00,
        7.2060525E+00, 5.8488586E+00, 2.7754916E+00, 7.7075166E-01,
        1.1582665E-01, 7.3138868E-03
},} ;
const struct poly tN1 = { -0.000, 7, {
          0.00000E+00, 3.86896E+01, -1.08267E+00, 4.70205E-02,
          -2.12169E-06, -1.17272E-04, 5.39280E-06, -7.98156E-08,
},} ;
const struct poly tN2 = { 20.613, 5, {
        1.972485E+01, 3.300943E+01, -3.915159E-01, 9.855391E-03,
        -1.274371E-04, 7.767022E-07
},} ;
struct polys tN = {3,{ &tN0, &tN1, &tN2, }} ;
struct thermo thermoN = { &vN, &tN, } ;

