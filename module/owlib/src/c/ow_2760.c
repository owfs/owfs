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

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
    ( or a closely related family of chips )

    The connection to the larger program is through the "device" data structure,
      which must be declared in the acompanying header file.

    The device structure holds the
      family code,
      name,
      number of properties,
      list of property structures, called "filetype".

    Each filetype structure holds the
      name,
      estimated length (in bytes),
      aggregate structure pointer,
      data format,
      read function,
      write funtion,
      generic data pointer

    The aggregate structure, is present for properties that several members
    (e.g. pages of memory or entries in a temperature log. It holds:
      number of elements
      whether the members are lettered or numbered
      whether the elements are stored together and split, or separately and joined
*/

/* Changes
    7/2004 Extensive improvements based on input from Serg Oskin
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_2760.h"

/* ------- Prototypes ----------- */

/* DS2406 switch */
READ_FUNCTION(FS_r_mem);
WRITE_FUNCTION(FS_w_mem);
READ_FUNCTION(FS_r_page);
WRITE_FUNCTION(FS_w_page);
READ_FUNCTION(FS_r_lock);
WRITE_FUNCTION(FS_w_lock);
READ_FUNCTION(FS_r_volt);
READ_FUNCTION(FS_r_temp);
READ_FUNCTION(FS_r_current);
READ_FUNCTION(FS_r_vis);
READ_FUNCTION(FS_r_vis_avg);
READ_FUNCTION(FS_r_pio);
WRITE_FUNCTION(FS_w_pio);
READ_FUNCTION(FS_r_vh);
WRITE_FUNCTION(FS_w_vh);
READ_FUNCTION(FS_r_vis_off);
WRITE_FUNCTION(FS_w_vis_off);
READ_FUNCTION(FS_r_ah);
WRITE_FUNCTION(FS_w_ah);
READ_FUNCTION(FS_r_abias);
WRITE_FUNCTION(FS_w_abias);
READ_FUNCTION(FS_r_templim);
WRITE_FUNCTION(FS_w_templim);
READ_FUNCTION(FS_r_voltlim);
WRITE_FUNCTION(FS_w_voltlim);
READ_FUNCTION(FS_r_vbias);
WRITE_FUNCTION(FS_w_vbias);
READ_FUNCTION(FS_r_timer);
WRITE_FUNCTION(FS_w_timer);
READ_FUNCTION(FS_r_bit);
WRITE_FUNCTION(FS_w_bit);
WRITE_FUNCTION(FS_charge);
WRITE_FUNCTION(FS_refresh);

#if OW_THERMOCOUPLE
READ_FUNCTION(FS_thermocouple);
READ_FUNCTION(FS_rangelow);
READ_FUNCTION(FS_rangehigh);
 /* ------- Structures ----------- */

struct thermocouple {
	_FLOAT v1, v2;
	_FLOAT rangeLow, rangeHigh;
	_FLOAT low[11];
	_FLOAT mid[11];
	_FLOAT high[11];
	_FLOAT mV[11];
};

struct thermocouple type_b = {
	0.437, 1.822, 0, 1820,
	{-3.34549e+09, 7.57222e+09, -7.34952e+09, 3.9973e+09, -1.3362e+09,
	 2.83366e+08, -3.80268e+07, 3.13466e+06, -151226, 4738.17, 31.9924,},
	{-149.905, 1643.07, -7937.72, 22240.5, -40004.8, 48278.9, -39657.6,
	 22027.5, -8104.23, 2140.22, -26.3893,},
	{-1.10165e-08, 1.13612e-06, -5.18188e-05, 0.00138477, -0.024161,
	 0.289846, -2.44837, 14.7621, -65.4035, 303.638, 201.601,},
	{3.4607e-21, -1.54436e-18, 2.33598e-16, -6.23624e-15, -2.18387e-12,
	 2.88144e-10, -1.60479e-08, 4.47121e-07, 1.95406e-07, -0.000227614,
	 8.70069e-05,},
};

struct thermocouple type_e = {
	-3.306, 9.86, -270, 1000,
	{-0.00186348, -0.119593, -3.40982, -56.8497, -613.48, -4475.37,
	 -22342.6, -75348.3, -164231, -208853, -117692,},
	{-2.29826e-09, 4.45093e-08, 2.57026e-07, -1.17477e-05, 8.44013e-05,
	 4.74888e-05, -0.00303068, 0.0185878, -0.248035, 17.057, 0.00354977,},
	{-1.23525e-15, 4.71812e-13, -7.8381e-11, 7.53249e-09, -4.72537e-07,
	 2.07906e-05, -0.000672767, 0.0167714, -0.340304, 17.683, -1.55323,},
	{-1.07611e-22, 1.31153e-20, 4.65438e-18, -5.31167e-16, -7.78362e-14,
	 7.82564e-12, 7.64931e-10, -1.21373e-07, 5.06679e-05, 0.0586129,
	 -0.000325966,},
};

struct thermocouple type_j = {
	14.055, 27.057, -210, 1200,
	{-1.86216e-09, 7.06203e-08, -7.40759e-07, -1.68076e-06, 5.62956e-05,
	 3.08928e-05, -0.00342003, 0.0233241, -0.217611, 19.8171, -0.0342962,},
	{1.04705e-09, -2.12282e-07, 1.92779e-05, -0.00103258, 0.0361236,
	 -0.8624, 14.2278, -160.156, 1177.08, -5082.05, 9897.53,},
	{4.75069e-13, -2.37677e-10, 5.28345e-08, -6.86486e-06, 0.000576832,
	 -0.0327315, 1.26987, -33.2645, 563.208, -5548.9, 24405.1,},
	{-6.97491e-24, 1.3658e-21, 2.04292e-19, -5.09902e-17, -1.49481e-15,
	 4.67638e-13, 1.34732e-10, -8.86546e-08, 3.04846e-05, 0.0503846,
	 -1.05193e-06,},
};

struct thermocouple type_k = {
	11.176, 22.691, -270, 1370,
	{-2.46084e-07, 6.81532e-06, -4.21632e-05, -0.000322014, 0.00355914,
	 0.0028323, -0.0906577, 0.124291, 0.192566, 24.9404, -0.722592,},
	{-2.14528e-09, 3.38578e-07, -2.36415e-05, 0.000959251, -0.0249579,
	 0.432955, -5.03485, 38.3162, -179.073, 470.662, -403.75,},
	{1.22956e-13, -4.95882e-11, 8.73994e-09, -8.91551e-07, 5.85688e-05,
	 -0.00259803, 0.0789352, -1.61945, 21.3873, -139.99, 562.639,},
	{-6.76827e-23, 4.59786e-21, 3.02777e-18, -1.80296e-16, -5.00204e-14,
	 2.58284e-12, 3.67046e-10, -1.31662e-07, 2.61205e-05, 0.0394427,
	 -0.000165505,},
};

struct thermocouple type_n = {
	-0.26, 7.631, -270, 1300,
	{-0.384204, -8.51012, -81.238, -437.501, -1461.59, -3138.2, -4341.49,
	 -3775.54, -1949.5, -493.211, -57.4264,},
	{1.79316e-06, -7.06517e-05, 0.00119405, -0.0113121, 0.0659068,
	 -0.244075, 0.574314, -0.793326, -0.362399, 38.3962, 0.0137761,},
	{1.50132e-13, -3.64746e-11, 3.78312e-09, -2.17623e-07, 7.49952e-06,
	 -0.000151796, 0.00137062, 0.0144983, -0.707806, 36.6116, 4.4746,},
	{-1.67472e-22, -5.75376e-21, 7.87952e-18, 2.66768e-16, -1.44225e-13,
	 -4.94289e-12, 1.51444e-09, 1.06098e-08, 1.15203e-05, 0.0260828,
	 -0.000928019,},
};

struct thermocouple type_r = {
	1.942, 12.687, -50, 1768,
	{-2.97833, 28.6494, -117.396, 268.213, -377.551, 347.099, -226.83,
	 130.345, -93.3971, 188.87, -0.0165138,},
	{-1.1534e-08, 1.07402e-06, -4.2822e-05, 0.000981669, -0.0145961,
	 0.149842, -1.09292, 5.68469, -22.0162, 151.516, 11.2428,},
	{-1.60073e-06, 0.00027281, -0.020812, 0.936039, -27.4906, 550.959,
	 -7632.14, 72164.2, -445780, 1.62479e+06, -2.65314e+06,},
	{-2.61253e-22, 5.71644e-20, 4.54127e-18, -1.99547e-15, 1.3374e-13,
	 3.09864e-12, -4.60676e-10, -1.99743e-08, 1.43025e-05, 0.00528674,
	 3.13725e-05,},
};

struct thermocouple type_s = {
	7.981, 11.483, -50, 1768,
	{-6.46807e-05, 0.00270224, -0.0485888, 0.492361, -3.09548, 12.5625,
	 -33.438, 59.3949, -76.2119, 186.708, -0.185908,},
	{0.00082531, -0.0846629, 3.88937, -105.403, 1866.54, -22574, 188861,
	 -1.07949e+06, 4.03486e+06, -8.90662e+06, 8.81855e+06,},
	{-1.42197e-05, 0.00217057, -0.148489, 5.99566, -158.252, 2853.22,
	 -35589.5, 303277, -1.68982e+06, 5.55955e+06, -8.20142e+06,},
	{-3.61972e-22, 1.85682e-19, -3.38826e-17, 2.28563e-15, 2.46683e-14,
	 -8.74753e-12, 2.22002e-10, -1.75089e-08, 1.24207e-05, 0.00540621,
	 2.31793e-05,},
};

struct thermocouple type_t = {
	-1.785, 2.556, -270, 400,
	{-0.0856873, -3.35591, -58.1948, -587.947, -3829.53, -16790.4,
	 -50151.1, -100705, -130042, -97452.3, -32202.6,},
	{0.000151427, 3.35406e-05, -0.00372056, 0.00326535, 0.022672,
	 -0.0222902, -0.0694538, 0.116881, -0.67161, 25.8257, -0.0024547,},
	{-5.44871e-11, 6.49228e-09, -3.38465e-07, 1.02385e-05, -0.000201171,
	 0.00272364, -0.0263712, 0.192645, -1.30813, 27.0402, -0.900659,},
	{-1.1498e-22, 7.18468e-21, 5.31691e-18, -2.88407e-16, -9.51445e-14,
	 4.02711e-12, 8.60657e-10, -5.74685e-08, 4.17544e-05, 0.0386747,
	 -0.000253129,},
};

#define F_thermocouple  \
{"typeB"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,    \
{"typeB/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile, {o:FS_thermocouple}, {v:NULL}  , {v:&type_b}, } , \
{"typeB/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangelow} , {v:NULL}     , {v:&type_b}, } , \
{"typeB/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangehigh}, {v:NULL}     , {v:&type_b}, } , \
    \
{"typeE"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,    \
{"typeE/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile, {o:FS_thermocouple}, {v:NULL}  , {v:&type_e}, } , \
{"typeE/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangelow} , {v:NULL}     , {v:&type_e}, } , \
{"typeE/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangehigh}, {v:NULL}     , {v:&type_e}, } , \
    \
{"typeJ"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,    \
{"typeJ/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile, {o:FS_thermocouple}, {v:NULL}  , {v:&type_j}, } , \
{"typeJ/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangelow} , {v:NULL}     , {v:&type_j}, } , \
{"typeJ/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangehigh}, {v:NULL}     , {v:&type_j}, } , \
    \
{"typeK"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,    \
{"typeK/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile, {o:FS_thermocouple}, {v:NULL}  , {v:&type_k}, } , \
{"typeK/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangelow} , {v:NULL}     , {v:&type_k}, } , \
{"typeK/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangehigh}, {v:NULL}     , {v:&type_k}, } , \
    \
{"typeN"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,    \
{"typeN/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile, {o:FS_thermocouple}, {v:NULL}  , {v:&type_n}, } , \
{"typeN/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangelow} , {v:NULL}     , {v:&type_n}, } , \
{"typeN/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangehigh}, {v:NULL}     , {v:&type_n}, } , \
    \
{"typeR"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,    \
{"typeR/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile, {o:FS_thermocouple}, {v:NULL}  , {v:&type_r}, } , \
{"typeR/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangelow} , {v:NULL}     , {v:&type_r}, } , \
{"typeR/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangehigh}, {v:NULL}     , {v:&type_r}, } , \
    \
{"typeS"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,    \
{"typeS/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile, {o:FS_thermocouple}, {v:NULL}  , {v:&type_s}, } , \
{"typeS/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangelow} , {v:NULL}     , {v:&type_s}, } , \
{"typeS/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangehigh}, {v:NULL}     , {v:&type_s}, } , \
    \
{"typeT"            ,PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir  , fc_volatile, {v:NULL}        , {v:NULL}     , {v:NULL}, } ,    \
{"typeT/temperature",PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_volatile, {o:FS_thermocouple}, {v:NULL}  , {v:&type_t}, } , \
{"typeT/range_low"  ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangelow} , {v:NULL}     , {v:&type_t}, } , \
{"typeT/range_high" ,PROPERTY_LENGTH_TEMP, NULL,ft_temperature, fc_static  , {o:FS_rangehigh}, {v:NULL}     , {v:&type_t}, } ,

#else							/* OW_THERMOCOUPLE */
#define F_thermocouple
#endif							/* OW_THERMOCOUPLE */

struct LockPage {
	int pages;
	size_t reg;
	size_t size;
	off_t offset[3];
};

#define Pages2720   2
#define Pages2751   2
#define Pages2755   3
#define Pages2760	2
#define Pages2770	3
#define Pages2780	2

#define Size2720	4
#define Size2751       16
#define Size2755       32
#define Size2760       16
#define Size2770       16
#define Size2780       16

struct LockPage P2720 =
	{ Pages2720, 0x07, Size2720, {0x20, 0x30, 0x00,}, };
struct LockPage P2751 =
	{ Pages2751, 0x07, Size2751, {0x20, 0x30, 0x00,}, };
struct LockPage P2755 =
	{ Pages2755, 0x07, Size2755, {0x20, 0x40, 0x60,}, };
struct LockPage P2760 =
	{ Pages2760, 0x07, Size2760, {0x20, 0x30, 0x00,}, };
struct LockPage P2770 =
	{ Pages2770, 0x07, Size2770, {0x20, 0x30, 0x40,}, };
struct LockPage P2780 =
	{ Pages2780, 0x1F, Size2780, {0x20, 0x60, 0x00,}, };

struct aggregate L2720 = { Pages2720, ag_numbers, ag_separate };
struct aggregate L2751 = { Pages2751, ag_numbers, ag_separate };
struct aggregate L2755 = { Pages2755, ag_numbers, ag_separate };
struct aggregate L2760 = { Pages2760, ag_numbers, ag_separate };
struct aggregate L2770 = { Pages2770, ag_numbers, ag_separate };
struct aggregate L2780 = { Pages2780, ag_numbers, ag_separate };

struct filetype DS2720[] = {
	F_STANDARD,
  {"lock",PROPERTY_LENGTH_YESNO, &L2720, ft_yesno, fc_stable, {o: FS_r_lock}, {o: FS_w_lock}, {v:&P2720},},
  {"memory", 256, NULL, ft_binary, fc_volatile, {o: FS_r_mem}, {o: FS_w_mem}, {v:NULL},},
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", Size2720, &L2720, ft_binary, fc_volatile, {o: FS_r_page}, {o: FS_w_page}, {v:&P2720},},

  {"cc",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x03},},
  {"ce",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x01},},
  {"dc",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x02},},
  {"de",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x00},},
  {"doc",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x04},},
  {"ot",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x08 << 8) | 0x00},},
  {"ov",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x07},},
  {"psf",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x08 << 8) |0x07},},
  {"uv",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x06},},
};

DeviceEntry(31, DS2720);

struct filetype DS2740[] = {
	F_STANDARD,
  {"memory", 256, NULL, ft_binary, fc_volatile, {o: FS_r_mem}, {o: FS_w_mem}, {v:NULL},},
  {"PIO",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_pio}, {o: FS_w_pio}, {u:(0x08 << 8) | 0x06},},
  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vis}, {v: NULL}, {v:NULL},},
  {"vis_B",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vis}, {v: NULL}, {f:1.5625E-6},},
  {"volthours",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vh}, {o: FS_w_vh}, {v:NULL},},

  {"smod",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x06},},
};

DeviceEntry(36, DS2740);

struct filetype DS2751[] = {
	F_STANDARD,
  {"amphours",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_ah}, {o: FS_w_ah}, {v:NULL},},
  {"current",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_current}, {v: NULL}, {v:NULL},},
  {"currentbias",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, {o: FS_r_abias}, {o: FS_w_abias}, {v:NULL},},
  {"lock",PROPERTY_LENGTH_YESNO, &L2751, ft_yesno, fc_stable, {o: FS_r_lock}, {o: FS_w_lock}, {v:&P2751},},
  {"memory", 256, NULL, ft_binary, fc_volatile, {o: FS_r_mem}, {o: FS_w_mem}, {v:NULL},},
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", Size2751, &L2751, ft_binary, fc_volatile, {o: FS_r_page}, {o: FS_w_page}, {v:&P2751},},
  {"PIO",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {v: NULL}, {o: FS_w_pio}, {u:(0x08 << 8) | 0x06},},
  {"sensed",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {v: NULL}, {u:(0x08 << 8) | 0x06},},
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_r_temp}, {v: NULL}, {v:NULL},},
  {"vbias",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, {o: FS_r_vbias}, {o: FS_w_vbias}, {v:NULL},},
  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vis}, {v: NULL}, {v:NULL},},
  {"volt",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_volt}, {v: NULL}, {s:0x0C},},
  {"volthours",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vh}, {o: FS_w_vh}, {v:NULL},},

  {"defaultpmod",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x31 << 8) | 0x05},},
  {"pmod",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {v: NULL}, {u:(0x01 << 8) | 0x05},},
  {"por",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x08 << 8) | 0x00},},
  {"uven",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x03},},

	F_thermocouple
};

DeviceEntry(51, DS2751);

struct filetype DS2755[] = {
	F_STANDARD,
  {"lock",PROPERTY_LENGTH_YESNO, &L2755, ft_yesno, fc_stable, {o: FS_r_lock}, {o: FS_w_lock}, {v:&P2751},},
  {"memory", 256, NULL, ft_binary, fc_volatile, {o: FS_r_mem}, {o: FS_w_mem}, {v:NULL},},
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", Size2755, &L2755, ft_binary, fc_volatile, {o: FS_r_page}, {o: FS_w_page}, {v:&P2755},},
  {"PIO",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {v: NULL}, {o: FS_w_pio}, {u:(0x08 << 8) | 0x06},},
  {"sensed",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {v: NULL}, {u:(0x08 << 8) | 0x06},},
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_r_temp}, {v: NULL}, {v:NULL},},
  {"vbias",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, {o: FS_r_vbias}, {o: FS_w_vbias}, {v:NULL},},
  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vis}, {v: NULL}, {v:NULL},},
  {"vis_avg",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vis_avg}, {v: NULL}, {v:NULL},},
  {"volt",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_volt}, {v: NULL}, {s:0x0C},},
  {"volthours",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vh}, {o: FS_w_vh}, {v:NULL},},

  {"alarm_set",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"alarm_set/volthigh",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_voltlim}, {o: FS_w_voltlim}, {u:0x80},},
  {"alarm_set/voltlow",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_voltlim}, {o: FS_w_voltlim}, {u:0x82},},
  {"alarm_set/temphigh",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_r_templim}, {o: FS_w_templim}, {u:0x84},},
  {"alarm_set/templow",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_r_templim}, {o: FS_w_templim}, {u:0x85},},

  {"defaultpmod",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x31 << 8) | 0x05},},
  {"pie1",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {v: NULL}, {u:(0x01 << 8) | 0x07},},
  {"pie0",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {v: NULL}, {u:(0x01 << 8) | 0x06},},
  {"pmod",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {v: NULL}, {u:(0x01 << 8) | 0x05},},
  {"rnaop",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {v: NULL}, {u:(0x01 << 8) | 0x04},},
  {"uven",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x03},},
  {"ios",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x02},},
  {"uben",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x01},},
  {"ovd",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x00},},
  {"por",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x08 << 8) | 0x00},},

	F_thermocouple
};

DeviceEntryExtended(35, DS2755, DEV_alarm);

struct filetype DS2760[] = {
	F_STANDARD,
  {"amphours",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_ah}, {o: FS_w_ah}, {v:NULL},},
  {"current",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_current}, {v: NULL}, {v:NULL},},
  {"currentbias",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, {o: FS_r_abias}, {o: FS_w_abias}, {v:NULL},},
  {"lock",PROPERTY_LENGTH_YESNO, &L2760, ft_yesno, fc_stable, {o: FS_r_lock}, {o: FS_w_lock}, {v:&P2760},},
  {"memory", 256, NULL, ft_binary, fc_volatile, {o: FS_r_mem}, {o: FS_w_mem}, {v:NULL},},
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", Size2760, &L2760, ft_binary, fc_volatile, {o: FS_r_page}, {o: FS_w_page}, {v:&P2760},},
  {"PIO",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {v: NULL}, {o: FS_w_pio}, {u:(0x08 << 8) | 0x06},},
  {"sensed",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {v: NULL}, {u:(0x08 << 8) | 0x06},},
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_r_temp}, {v: NULL}, {s:0x18},},
  {"vbias",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, {o: FS_r_vbias}, {o: FS_w_vbias}, {v:NULL},},
  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vis}, {v: NULL}, {v:NULL},},
  {"volt",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_volt}, {v: NULL}, {s:0x0C},},
  {"volthours",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vh}, {o: FS_w_vh}, {v:NULL},},

  {"cc",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x03},},
  {"ce",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x01},},
  {"coc",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x05},},
  {"defaultpmod",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x31 << 8) | 0x05},},
  {"defaultswen",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x31 << 8) | 0x03},},
  {"dc",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x02},},
  {"de",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x00},},
  {"doc",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x04},},
  {"mstr",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {v: NULL}, {u:(0x08 << 8) | 0x05},},
  {"ov",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x07},},
  {"ps",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x08 << 8) | 0x07},},
  {"pmod",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {v: NULL}, {u:(0x01 << 8) | 0x05},},
  {"swen",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {v: NULL}, {u:(0x01 << 8) | 0x03},},
  {"uv",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x00 << 8) | 0x06},},

	F_thermocouple
};

DeviceEntry(30, DS2760);

struct filetype DS2770[] = {
	F_STANDARD,
  {"amphours",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_ah}, {o: FS_w_ah}, {v:NULL},},
  {"current",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_current}, {v: NULL}, {v:NULL},},
  {"currentbias",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, {o: FS_r_abias}, {o: FS_w_abias}, {v:NULL},},
  {"lock",PROPERTY_LENGTH_YESNO, &L2770, ft_yesno, fc_stable, {o: FS_r_lock}, {o: FS_w_lock}, {v:&P2770},},
  {"memory", 256, NULL, ft_binary, fc_volatile, {o: FS_r_mem}, {o: FS_w_mem}, {v:NULL},},
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", Size2770, &L2770, ft_binary, fc_volatile, {o: FS_r_page}, {o: FS_w_page}, {v:&P2770},},
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_r_temp}, {v: NULL}, {v:NULL},},
  {"vbias",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, {o: FS_r_vbias}, {o: FS_w_vbias}, {v:NULL},},
  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vis}, {v: NULL}, {v:NULL},},
  {"volt",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_volt}, {v: NULL}, {s:0x0C},},
  {"volthours",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vh}, {o: FS_w_vh}, {v:NULL},},

  {"charge",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {v: NULL}, {o: FS_charge}, {v:NULL},},
  {"cini",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x01},},
  {"cstat1",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x07},},
  {"cstat0",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x06},},
  {"ctype",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x00},},
  {"defaultpmod",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x31 << 8) | 0x05},},
  {"pmod",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x05},},
  {"refresh",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_stable, {v: NULL}, {o: FS_refresh}, {v:NULL},},
  {"timer",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_timer}, {o: FS_w_timer}, {v:NULL},},

	F_thermocouple
};

DeviceEntry(2E, DS2770);

struct filetype DS2780[] = {
	F_STANDARD,
  {"lock",PROPERTY_LENGTH_YESNO, &L2780, ft_yesno, fc_stable, {o: FS_r_lock}, {o: FS_w_lock}, {v:&P2780},},
  {"memory", 256, NULL, ft_binary, fc_volatile, {o: FS_r_mem}, {o: FS_w_mem}, {v:NULL},},
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", Size2780, &L2780, ft_binary, fc_volatile, {o: FS_r_page}, {o: FS_w_page}, {v:&P2780},},
  {"PIO",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {v: NULL}, {o: FS_w_pio}, {u:(0x15 << 8) | 0x00},},
  {"sensed",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {v: NULL}, {u:(0x15 << 8) | 0x00},},
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_r_temp}, {v: NULL}, {v:NULL},},
  {"vbias",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, {o: FS_r_vbias}, {o: FS_w_vbias}, {v:NULL},},
  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vis}, {v: NULL}, {v:NULL},},
  {"vis_avg",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vis_avg}, {v: NULL}, {v:NULL},},
  {"volt",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_volt}, {v: NULL}, {s:0x0C},},
  {"volthours",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vh}, {o: FS_w_vh}, {v:NULL},},

  {"aef",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x06},},
  {"chgtf",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x07},},
  {"learnf",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x04},},
  {"pmod",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x60 << 8) | 0x05},},
  {"porf",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x01},},
  {"sef",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x05},},
  {"uven",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x60 << 8) | 0x06},},
  {"uvf",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x02},},

	F_thermocouple
};

DeviceEntry(32, DS2780);

struct filetype DS2781[] = {
	F_STANDARD,
  {"lock",PROPERTY_LENGTH_YESNO, &L2780, ft_yesno, fc_stable, {o: FS_r_lock}, {o: FS_w_lock}, {v:&P2780},},
  {"memory", 256, NULL, ft_binary, fc_volatile, {o: FS_r_mem}, {o: FS_w_mem}, {v:NULL},},
  {"pages",PROPERTY_LENGTH_SUBDIR, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", Size2780, &L2780, ft_binary, fc_volatile, {o: FS_r_page}, {o: FS_w_page}, {v:&P2780},},
  {"PIO",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {v: NULL}, {o: FS_w_pio}, {u:(0x15 << 8) | 0x00},},
  {"sensed",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {v: NULL}, {u:(0x15 << 8) | 0x00},},
  {"temperature",PROPERTY_LENGTH_TEMP, NULL, ft_temperature, fc_volatile, {o: FS_r_temp}, {v: NULL}, {v:NULL},},
  {"vbias",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_stable, {o: FS_r_vbias}, {o: FS_w_vbias}, {v:NULL},},
  {"vis",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vis}, {v: NULL}, {v:NULL},},
  {"vis_offset",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vis_off}, {o: FS_w_vis_off}, {v:NULL},},
  {"vis_avg",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vis_avg}, {v: NULL}, {v:NULL},},
  {"volt",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_volt}, {v: NULL}, {s:0x0C},},
  {"volthours",PROPERTY_LENGTH_FLOAT, NULL, ft_float, fc_volatile, {o: FS_r_vh}, {o: FS_w_vh}, {v:NULL},},

  {"aef",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x06},},
  {"chgtf",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x07},},
  {"learnf",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x04},},
  {"pmod",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x60 << 8) | 0x05},},
  {"porf",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x01},},
  {"sef",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x05},},
  {"uven",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x60 << 8) | 0x06},},
  {"uvf",PROPERTY_LENGTH_YESNO, NULL, ft_yesno, fc_volatile, {o: FS_r_bit}, {o: FS_w_bit}, {u:(0x01 << 8) | 0x02},},

	F_thermocouple
};

DeviceEntry(3D, DS2781);

/* ------- Functions ------------ */

/* DS2406 */
static int OW_r_sram(BYTE * data, const size_t size, const off_t offset,
					 const struct parsedname *pn);
static int OW_w_sram(const BYTE * data, const size_t size,
					 const off_t offset, const struct parsedname *pn);
static int OW_r_mem(BYTE * data, const size_t size, const off_t offset,
					const struct parsedname *pn);
static int OW_w_mem(const BYTE * data, const size_t size,
					const off_t offset, const struct parsedname *pn);
static int OW_r_eeprom(const size_t page, const size_t size,
					   const off_t offset, const struct parsedname *pn);
static int OW_w_eeprom(const size_t page, const size_t size,
					   const off_t offset, const struct parsedname *pn);
static int OW_lock(const struct parsedname *pn);
static int OW_r_int(int *I, off_t offset, const struct parsedname *pn);
static int OW_w_int(const int *I, off_t offset,
					const struct parsedname *pn);
static int OW_r_int8(int *I, off_t offset, const struct parsedname *pn);
static int OW_w_int8(const int *I, off_t offset,
					 const struct parsedname *pn);
static int OW_cmd(const BYTE cmd, const struct parsedname *pn);
}


/* 2406 memory read */
static int FS_r_mem(struct one_wire_query * owq)
{
	/* read is not a "paged" endeavor, the CRC comes after a full read */
    if (OW_r_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), PN(owq)))
		return -EINVAL;
    return OWQ_size(owq);
}

/* 2406 memory write */
static int FS_r_page(struct one_wire_query * owq)
{
//printf("2406 read size=%d, offset=%d\n",(int)size,(int)offset);
    if (OW_r_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq) +
        ((struct LockPage *) OWQ_pn(owq).ft->data.v)->offset[OWQ_pn(owq).extension], PN(owq)))
		return -EINVAL;
    return OWQ_size(owq);
}

static int FS_w_page(struct one_wire_query * owq)
{
	if (OW_w_mem
           ((BYTE *) OWQ_buffer(owq), OWQ_size(owq),
            OWQ_offset(owq) +
                    ((struct LockPage *) OWQ_pn(owq).ft->data.v)->offset[OWQ_pn(owq).extension], PN(owq)))
		return -EINVAL;
	return 0;
}

/* 2406 memory write */
static int FS_r_lock(struct one_wire_query * owq)
{
	BYTE data;
    if (OW_r_mem(&data, 1, ((struct LockPage *) OWQ_pn(owq).ft->data.v)->reg, PN(owq)))
		return -EINVAL;
    OWQ_Y(owq) = UT_getbit(&data, OWQ_pn(owq).extension);
	return 0;
}

static int FS_w_lock(struct one_wire_query * owq)
{
    if (OW_lock(PN(owq)))
		return -EINVAL;
	return 0;
}

/* Note, it's EPROM -- write once */
static int FS_w_mem(struct one_wire_query * owq)
{
	/* write is "byte at a time" -- not paged */
    if (OW_w_mem((BYTE *) OWQ_buffer(owq), OWQ_size(owq), OWQ_offset(owq), PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_r_vis(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int I;
	_FLOAT f = 0.;
	if (OW_r_int(&I, 0x0E, pn))
		return -EINVAL;
	switch (pn->sn[0]) {
	case 0x36:					//DS2740
		f = 6.25E-6;
    	if (pn->ft->data.v)
    		f = pn->ft->data.f;		// for DS2740BU
		break;
	case 0x51:					//DS2751
	case 0x35:					//DS2755
	case 0x30:					//DS2760
		f = 15.625E-6 / 8;		// Jan Bertelsen's correction
		break;
	case 0x28:					//DS2770
		f = 1.56E-6;
		break;
	case 0x32:					//DS2780
	case 0x3D:					//DS2781
		f = 1.5625E-6;
		break;
	}
    OWQ_F(owq) = f * I;
	return 0;
}

// Volt-hours
static int FS_r_vis_avg(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int I;
	int ret = 1;
	switch (pn->sn[0]) {
	case 0x32:					//DS2780
	case 0x3D:					//DS2781
		ret = OW_r_int(&I, 0x08, pn);
        OWQ_F(owq) = .0000015625 * I;
		break;
	case 0x35:					//DS2755
		ret = OW_r_int(&I, 0x1A, pn);
        OWQ_F(owq) = .000001953 * I;
		break;
	}
	return ret ? -EINVAL : 0;
}

// Volt-hours
static int FS_r_vh(struct one_wire_query * owq)
{
	int I;
    if (OW_r_int(&I, 0x10, PN(owq)))
		return -EINVAL;
    OWQ_F(owq) = .00000625 * I;
	return 0;
}

// Volt-hours
static int FS_w_vh(struct one_wire_query * owq)
{
    int I = OWQ_F(owq) / .00000625;
    return OW_w_int(&I, 0x10, PN(owq)) ? -EINVAL : 0;
}

// Volt-limits
static int FS_r_voltlim(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int I;
	if (OW_r_int(&I, pn->ft->data.u, pn))
		return -EINVAL;
    OWQ_F(owq) = .00000625 * I;
	return 0;
}

// Volt-limits
static int FS_w_voltlim(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int I = OWQ_F(owq) / .00000625;
	return OW_w_int(&I, pn->ft->data.u, pn) ? -EINVAL : 0;
}

// Volt-limits
static int FS_r_templim(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int I;
	if (OW_r_int8(&I, pn->ft->data.u, pn))
		return -EINVAL;
    OWQ_F(owq) = I;
	return 0;
}

// Volt-limits
static int FS_w_templim(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int I = OWQ_F(owq);
	if (I < -128)
		return -ERANGE;
	if (I > 127)
		return -ERANGE;
	return OW_w_int8(&I, pn->ft->data.u, pn) ? -EINVAL : 0;
}

// timer
static int FS_r_timer(struct one_wire_query * owq)
{
	int I;
    if (OW_r_int(&I, 0x02, PN(owq)))
		return -EINVAL;
    OWQ_F(owq) = .015625 * I;
	return 0;
}

// timer
static int FS_w_timer(struct one_wire_query * owq)
{
    int I = OWQ_F(owq) / .015625;
    return OW_w_int(&I, 0x02, PN(owq)) ? -EINVAL : 0;
}

// Amp-hours -- using 25mOhm internal resistor
static int FS_r_ah(struct one_wire_query * owq)
{
    if (FS_r_vh(owq))
		return -EINVAL;
    OWQ_F(owq) = OWQ_F(owq) / .025;
	return 0;
}

// Amp-hours -- using 25mOhm internal resistor
static int FS_w_ah(struct one_wire_query * owq)
{
    OWQ_F(owq) = OWQ_F(owq) * .025;
    return FS_w_vh(owq);
}

// current offset
static int FS_r_vis_off(struct one_wire_query * owq)
{
	int I;
    if (OW_r_int8(&I, 0x7B, PN(owq)))
		return -EINVAL;
    OWQ_F(owq) = 1.56E-6 * I;
	return 0;
}

// current offset
static int FS_w_vis_off(struct one_wire_query * owq)
{
    int I = OWQ_F(owq) / 1.56E-6;
	if (I < -128)
		return -ERANGE;
	if (I > 127)
		return -ERANGE;
    return OW_w_int8(&I, 0x7B, PN(owq)) ? -EINVAL : 0;
}

// Current bias -- using 25mOhm internal resistor
static int FS_r_abias(struct one_wire_query * owq)
{
    int ret = FS_r_vbias(owq);
    if ( ret ) return ret ;
    OWQ_F(owq) = OWQ_F(owq) / .025;
	return 0;
}

// Current bias -- using 25mOhm internal resistor
static int FS_w_abias(struct one_wire_query * owq)
{
    OWQ_F(owq) = OWQ_F(owq) * .025;
    return FS_w_vbias(owq);
}

// Read current using internal 25mOhm resistor and Vis
static int FS_r_current(struct one_wire_query * owq)
{
    int ret = FS_r_vis(owq);
    OWQ_F(owq) = OWQ_F(owq) / .025;
	return ret;
}

static int FS_r_vbias(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int I;
	switch (pn->sn[0]) {
	case 0x51:					//DS2751
	case 0x30:					//DS2760
		if (OW_r_int8(&I, 0x33, pn))
			return -EINVAL;
        OWQ_F(owq) = 15.625E-6 * I;
		break;
	case 0x35:					//DS2755
		if (OW_r_int8(&I, 0x33, pn))
			return -EINVAL;
        OWQ_F(owq) = 1.95E-6 * I;
		break;
	case 0x28:					//DS2770
		// really a 2byte value!
		if (OW_r_int(&I, 0x32, pn))
			return -EINVAL;
        OWQ_F(owq) = 1.5625E-6 * I;
		break;
	case 0x32:					//DS2780
	case 0x3D:					//DS2780
		if (OW_r_int8(&I, 0x61, pn))
			return -EINVAL;
        OWQ_F(owq) = 1.5625E-6 * I;
		break;
	}
	return 0;
}

static int FS_w_vbias(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int I;
	int ret = 0;				// assign unnecessarily to avoid compiler warning
	switch (pn->sn[0]) {
	case 0x51:					//DS2751
	case 0x30:					//DS2760
        I = OWQ_F(owq) / 15.625E-6;
		if (I < -128)
			return -ERANGE;
		if (I > 127)
			return -ERANGE;
		ret = OW_w_int8(&I, 0x33, pn);
		break;
	case 0x35:					//DS2755
        I = OWQ_F(owq) / 1.95E-6;
		if (I < -128)
			return -ERANGE;
		if (I > 127)
			return -ERANGE;
		ret = OW_w_int8(&I, 0x33, pn);
		break;
	case 0x28:					//DS2770
        I = OWQ_F(owq) / 1.5625E-6;
		if (I < -32768)
			return -ERANGE;
		if (I > 32767)
			return -ERANGE;
		// really 2 bytes
		ret = OW_w_int(&I, 0x32, pn);
		break;
	case 0x32:					//DS2780
        I = OWQ_F(owq) / 1.5625E-6;
		if (I < -128)
			return -ERANGE;
		if (I > 127)
			return -ERANGE;
		ret = OW_w_int8(&I, 0x61, pn);
		break;
	}
	return ret ? -EINVAL : 0;
}

static int FS_r_volt(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int I;
	if (OW_r_int(&I, 0x0C, pn))
		return -EINVAL;
	switch (pn->sn[0]) {
	case 0x3D:					//DS2781
        OWQ_F(owq) = (I / 32) * .00976;
		break;
	default:
        OWQ_F(owq) = (I / 32) * .00488;
		break;
	}
	return 0;
}

static int FS_r_temp(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int I;
	size_t off;
	switch (pn->sn[0]) {
	case 0x32:					//DS2780
	case 0x3D:					//DS2781
		off = 0x0A;
		break;
	default:
		off = 0x18;
	}
	if (OW_r_int(&I, off, pn))
		return -EINVAL;
    OWQ_F(owq) = (I / 32) * .125;
	return 0;
}

static int FS_r_bit(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int bit = pn->ft->data.u & 0xFF;
	size_t location = pn->ft->data.u >> 8;
	BYTE c[1];
	if (OW_r_mem(c, 1, location, pn))
		return -EINVAL;
    OWQ_Y(owq) = UT_getbit(c, bit);
	return 0;
}

static int FS_w_bit(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    int bit = pn->ft->data.u & 0xFF;
	size_t location = pn->ft->data.u >> 8;
	BYTE c[1];
	if (OW_r_mem(c, 1, location, pn))
		return -EINVAL;
    UT_setbit(c, bit, OWQ_Y(owq) != 0);
	return OW_w_mem(c, 1, location, pn) ? -EINVAL : 0;
}

/* 2406 switch PIO sensed*/
static int FS_r_pio(struct one_wire_query * owq)
{
	if (FS_r_bit(owq))
		return -EINVAL;
    OWQ_Y(owq) = !OWQ_Y(owq);
	return 0;
}

static int FS_charge(struct one_wire_query * owq)
{
    if (OW_cmd(OWQ_Y(owq) ? 0xB5 : 0xBE, PN(owq)))
		return -EINVAL;
	return 0;
}

static int FS_refresh(struct one_wire_query * owq)
{
    if (OW_cmd(0x63, PN(owq)))
		return -EINVAL;
	return 0;
}

/* write PIO -- bit 6 */
static int FS_w_pio(struct one_wire_query * owq)
{
    OWQ_Y(owq) = !OWQ_Y(owq);
    return FS_w_bit(owq);
}

static int OW_r_eeprom(const size_t page, const size_t size,
					   const off_t offset, const struct parsedname *pn)
{
	BYTE p[] = { 0xB8, page & 0xFF, };
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 2, trxn_match,},
		TRXN_END,
	};
	if (offset > (off_t) page + 16)
		return 0;
	if (offset + size <= page)
		return 0;
	return BUS_transaction(t, pn);
}

/* just read the sram -- eeprom may need to be recalled if you want it */
static int OW_r_sram(BYTE * data, const size_t size, const off_t offset,
					 const struct parsedname *pn)
{
	BYTE p[] = { 0x69, offset & 0xFF, };
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 2, trxn_match,},
		{NULL, data, size, trxn_read,},
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static int OW_r_mem(BYTE * data, const size_t size, const off_t offset,
					const struct parsedname *pn)
{
	return OW_r_eeprom(0x20, size, offset, pn)
		|| OW_r_eeprom(0x30, size, offset, pn)
		|| OW_r_sram(data, size, offset, pn);
}

/* Special processing for eeprom -- page is the address of a 16 byte page (e.g. 0x20) */
static int OW_w_eeprom(const size_t page, const size_t size,
					   const off_t offset, const struct parsedname *pn)
{
	BYTE p[] = { 0x48, page & 0xFF, };
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 2, trxn_match,},
		TRXN_END,
	};
	if (offset > (off_t) page + 16)
		return 0;
	if (offset + size <= page)
		return 0;
	return BUS_transaction(t, pn);
}

static int OW_w_sram(const BYTE * data, const size_t size,
					 const off_t offset, const struct parsedname *pn)
{
	BYTE p[] = { 0x6C, offset & 0xFF, };
	struct transaction_log t[] = {
		TRXN_START,
		{p, NULL, 2, trxn_match,},
		{data, NULL, size, trxn_match,},
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static int OW_cmd(const BYTE cmd, const struct parsedname *pn)
{
	struct transaction_log t[] = {
		TRXN_START,
		{&cmd, NULL, 1, trxn_match,},
		TRXN_END,
	};

	return BUS_transaction(t, pn);
}

static int OW_w_mem(const BYTE * data, const size_t size,
					const off_t offset, const struct parsedname *pn)
{
	return OW_r_eeprom(0x20, size, offset, pn)
		|| OW_r_eeprom(0x30, size, offset, pn)
		|| OW_w_sram(data, size, offset, pn)
		|| OW_w_eeprom(0x20, size, offset, pn)
		|| OW_w_eeprom(0x30, size, offset, pn);
}

static int OW_r_int(int *I, off_t offset, const struct parsedname *pn)
{
	BYTE i[2];
	int ret = OW_r_sram(i, 2, offset, pn);
	if (ret)
		return ret;
		// Data in the DS27xx is stored MSB/LSB -- different from DS2438 for example
		// Thanks to Jan Bertelsen for finding this!
	I[0] = (((int)((int8_t)i[0])<<8) + ((uint8_t)i[1]) ) ;
	return 0;
}

static int OW_w_int(const int *I, off_t offset,
					const struct parsedname *pn)
{
	BYTE i[2] = { (I[0] >> 8) & 0xFF, I[0] & 0xFF, };
	return OW_w_sram(i, 2, offset, pn);
}

static int OW_r_int8(int *I, off_t offset, const struct parsedname *pn)
{
	BYTE i[1];
	int ret = OW_r_sram(i, 1, offset, pn);
	if (ret)
		return ret;
	//I[0] = ((int)((int8_t)i[0]) ) ;
	I[0] = UT_int8(i);
	return 0;
}

static int OW_w_int8(const int *I, off_t offset,
					 const struct parsedname *pn)
{
	BYTE i[1] = { I[0] & 0xFF, };
	return OW_w_sram(i, 1, offset, pn);
}

static int OW_lock(const struct parsedname *pn)
{
	BYTE lock[] = { 0x6C, 0x07, 0x40, 0x6A, 0x00 };
	struct transaction_log t[] = {
		TRXN_START,
		{lock, NULL, 5, trxn_match,},
		TRXN_END,
	};

	UT_setbit(&lock[2], pn->extension, 1);
	lock[4] = ((struct LockPage *) pn->ft->data.v)->offset[pn->extension];

	return BUS_transaction(t, pn);
}

#if OW_THERMOCOUPLE
static _FLOAT polycomp(_FLOAT x, _FLOAT * coef);

static int FS_rangelow(struct one_wire_query * owq)
{
    OWQ_F(owq) = ((struct thermocouple *) OWQ_pn(owq).ft->data.v)->rangeLow;
	return 0;
}

static int FS_rangehigh(struct one_wire_query * owq)
{
    OWQ_F(owq) = ((struct thermocouple *) OWQ_pn(owq).ft->data.v)->rangeHigh;
	return 0;
}

static int FS_thermocouple(struct one_wire_query * owq)
{
    struct parsedname * pn = PN(owq) ;
    _FLOAT T, V;
	int ret;
	struct thermocouple *thermo = (struct thermocouple *) pn->ft->data.v;

	/* Get reference temperature */
	if ((ret = FS_r_temp(owq)))
		return ret;				/* in C */
    T = OWQ_F(owq) ;
    
	/* Get measured voltage */
	if ((ret = FS_r_vis(owq)))
		return ret;
    V = OWQ_F(owq) * 1000;					/* convert Volts to mVolts */

	/* Correct voltage by adding reference temperature voltage */
	V += polycomp(T, thermo->mV);

	/* Find right range, them compute temparature from voltage */
	if (V < thermo->v1) {
        OWQ_F(owq) = polycomp(V, thermo->low);
	} else if (V < thermo->v2) {
        OWQ_F(owq) = polycomp(V, thermo->mid);
	} else {
        OWQ_F(owq) = polycomp(V, thermo->high);
	}

	return 0;
}

/* Compute a 10th order polynomial with cooef being a10, a9, ... a0 */
static _FLOAT polycomp(_FLOAT x, _FLOAT * coef)
{
	_FLOAT r = coef[0];
	int i;
	for (i = 1; i <= 10; ++i)
		r = x * r + coef[i];
	return r;
}
#endif							/* OW_THERMOCOUPLE */
