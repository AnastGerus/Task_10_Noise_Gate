#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>
#include <math.h>
#include "Fixed_math32.h"


int32_t float2fixed(double x) {

	int32_t res;

	if (x >= 1) {
		res = INT32MAX;
	}
	else if (x< (-1)) {
		res = INT32MIN;
	}
	else {
		double base = (double)BASE32;
		res = (int32_t)(x * base);
	}
	return (int32_t)res;
}

float fixed2float(int32_t x) {
	double base = (double)BASE32;
	double res = x / base;
	return (float)res;
}

double fixed2float64(int64_t x) {
	x = LeftShift64(x, 1);
	x = x >> 32;
	double base = (double)BASE32;
	double res = x / base;
	return (double)res;
}

double fixed2float16(int16_t x) {
	double base = (double)BASE16;
	double res = x / base;
	return (double)res;
}

int16_t float2fixed16(double x) {

	int16_t res;

	if (x >= 1) {
		res = INT16MAX;
	}
	else if (x< (-1)) {
		res = INT16MIN;
	}
	else {
		double base = (double)BASE16;
		res = (int16_t)(x * base);
	}
	return (int16_t)res;
}


int16_t LeftShift16(int16_t x, int n) {
	int16_t R;

	if (n >= 15) {
		if (x > 0) {
			return INT16MAX;
		}
		else return INT16MIN;
	}

	R = x << n;

	if ((R>x) && (x<0)) {
		R = INT16MIN;
	}
	else if ((R<x) && (x>0)) {
		R = INT16MAX;
	}

	return R;
}

int32_t LeftShift32(int32_t x, int n) {
	int32_t R;

	if (n >= 31) {
		if (x > 0) {
			return INT32MAX;
		}
		else return INT32MIN;
	}

	R = x << n;

	if ((R>x) && (x<0)) {
		R = INT32MIN;
	}
	else if ((R<x) && (x>0)) {
		R = INT32MAX;
	}

	return R;
}

int32_t RightShift32(int32_t x, int n) {
	int32_t R;

	if (n >= 31) {
		return 0;
	}

	R = x >> n;
	return R;
}

int64_t LeftShift64(int64_t x, int n) {
	int64_t R;

	if (n >= 63) {
		if (x > 0) {
			return INT64MAX;
		}
		else return INT64MIN;
	}

	R = x << n;

	if ((R>x) && (x<0)) {
		R = INT64MIN;
	}
	else if ((R<x) && (x>0)) {
		R = INT64MAX;
	}

	return R;
}

int64_t RightShift64(int64_t x, int n) {
	int64_t R;

	if (n >= 63) {
		return 0;
	}

	R = x >> n;
	return R;
}


int64_t SaturationResult32(int64_t x) {

	if (x > INT32MAX) {
		x = INT32MAX;
	}
	if (x < INT32MIN) {
		x = INT32MIN;
	}
	return x;
}

int32_t adds32(int32_t A, int32_t B) {

	int64_t R;
	R = (int64_t)A + B;

	R = SaturationResult32(R);

	return (int32_t)R;
}

int32_t subs32(int32_t A, int32_t B) {

	int64_t R;
	R = (int64_t)A - B;

	R = SaturationResult32(R);

	return (int32_t)R;
}

int64_t adds64(int64_t A, int64_t B) {

	int64_t R;
	R = A + B;

	//Saturation Result 64  Check for Addition
	if (((A^B) & INT64MIN) == 0)
	{
		if ((R^A) & INT64MIN)
		{
			R = (A<0) ? INT64MIN : INT64MAX;
		}
	}
	return R;
}

int64_t subs64(int64_t A, int64_t B) {

	int64_t R;
	R = A - B;

	//Saturation Result 64  Check for Subtraction
	if (((A^B) & INT64MIN) != 0)
	{
		if ((R^A) & INT64MIN)
		{
			R = (A<0) ? INT64MIN : INT64MAX;
		}
	}
	return R;
}


int32_t get_Hi31(int64_t x) {  // Q63  take hi 31

	x = RightShift64(x, 32);

	return (int32_t)x;
}

int32_t get_Low31(int64_t x) {  

	x = x & 0x000000007fffffff;

	return (int32_t)x;
}

int32_t get_Low30(int64_t x) {

	x = x & 0x000000003fffffff;

	return (int32_t)x;
}

int16_t get_Hi15(int32_t x) {  
	// Q31 take hi 15

	x = RightShift32(x, 16);

	return (int16_t)x;
}

int16_t get_Low15(int32_t x) {

	x = x & 0x7fff;

	return (int16_t)x;
}

int16_t get_Low14(int32_t x) {

	x = x & 0x3fff;

	return (int16_t)x;
}

int16_t get_Hi15_31(int64_t x) {  
	//Q63 take hi 15

	x = RightShift64(x, 48);

	return (int16_t)x;
}


int16_t Round64_15(int64_t x) {
	//Q63 -> Q15 

	x = get_Hi31(x);
	x += FIX05S;
	x = SaturationResult32(x);
	x = get_Hi15(x);
	return (int16_t)x;
}

int16_t Round32(int64_t x) {
	//for Q31

	x += FIX05S;
	x = SaturationResult32(x);
	x = RightShift32(x, 16);
	return (int16_t)x;
}

int32_t Round62(int64_t x) {
	//for Q62

	x = adds64(x, FIX05);
	x = RightShift64(x, 32);

	return (int32_t)x;
}

int32_t Round64(int64_t x) {
	//for Q63

	x = adds64(x, (1 << 31));
	x = RightShift64(x, 32);

	return (int32_t)x;
}


int64_t mul64(int32_t A, int32_t B) {
	int64_t R;

	R = (int64_t)A * B;

	return R;
}

int32_t mul32(int32_t A, int32_t B) {  //with Round! -> Q63
	int64_t R;
	int32_t Res;

	R = mul64(A, B);
	R = LeftShift64(R, 1);
	Res = Round64(R);

	return Res;
}

int32_t mul16(int16_t A, int16_t B) {
	int32_t R;

	R = (int32_t)A * B;

	return R;
}


int64_t mac32(int64_t acc, int32_t A, int32_t B) { //returns not compensated value in Q62

	acc = adds64(acc, mul64(A, B));
	return acc;
}

int64_t msub32(int64_t acc, int32_t A, int32_t B) {

	acc = subs64(acc, mul64(A, B));
	return acc;
}

int32_t mac16(int32_t acc, int16_t A, int16_t B) {

	acc = adds32(acc, mul16(A, B));
	return acc;
}


int64_t abs64(int64_t x) {
	if (x < 0) {
		if (x == INT64MIN) {
			return INT64MAX;
		}
		else return -x;
	}
	else return x;
}

int32_t abs32(int32_t x) {
	if (x < 0) {
		if (x == INT32MIN) {
			return INT32MAX;
		}
		else return -x;
	}
	else return x;
}

int16_t abs16(int16_t x) {
	if (x < 0) {
		if (x == INT16MIN) {
			return INT16MAX;
		}
		else return -x;
	}
	else return x;
}

//Division

double div_double(double x, double y) {
	// works for all numbers

	int i = 0;
	double temp1;

	while (y < 0.5) {
		y *= 2;
		i++;
	}

	for (; i > 0; i--) {
		x *= 2;
	}

	double coef1 = 0.4705882352941176;						//Q31 divided by 4  32/17
	double coef2 = 0.7058823529411765;

	temp1 = coef2 - coef1*y;
	temp1 *= 4;

	for (int e = 0; e < 3; e++) {
		temp1 = temp1 + temp1 * (1 - y * temp1);         // X + X *( 1 - D' * X)
	}

	return (x * temp1);
}

int Norm16(uint16_t x) {
	//Returns the number of (leading zeros-1) = number of needed shifts
	//INCREASING THE NUMBER TO THE RANGE (0.5; 1)
	int i = 0;

	if (x == 0) {
		return 0;
	}

	while (x <= (1 << 14))
	{
		x = x << 1;
		i++;
	}

	return i;
}

int16_t div16(int16_t x, int16_t y) // x - Numerator, y - Denumenator
{
	// Newton–Raphson division method
	// for small num / big num
	// for numbers < 1

	uint32_t coef1, coef2, one, ux, uy;
	uint32_t temp1;
	uint64_t temp, temp2, res;

	int16_t res_sig;
	int i = 0;

	if (x > y) {
		printf("Error! You can divide just a small number by bigger number!\n");
		system("pause");
		return 0;
	}
	if (y == 0) {
		printf("Error! Division by zero!\n");
		system("pause");
		return 0;
	}
	if (x == y) {
		return INT16MAX;
	}

	ux = (uint32_t)x; // N'
	uy = (uint32_t)y; // D'

	i = Norm16(uy);
	uy = uy << i;
	ux = ux << i;

	uy = LeftShift32(uy, 14);    //Q29
	ux = LeftShift32(ux, 14);

	coef1 = float2fixed(0.4705882352941176);  // Q31 divided by 4		32/17    
	coef2 = float2fixed(0.7058823529411765);  // Q31 divided by 4	    48/17   

	coef1 = RightShift32(coef1, 2);    //Q29
	coef2 = RightShift32(coef2, 2);
	one = RightShift32(INT32MAX, 2);

	//Initial approximation
	temp = mul64(coef1, uy);					// Q29 * Q29 = Q58,
	temp = adds64(temp, (1 << 28)); // round
	temp = RightShift64(temp, 29);  // -> Q29
	temp1 = subs32(coef2, (int32_t)temp);		// X in Q29
												// X := 48/17 ? 32/17 × D'

	temp1 = LeftShift32(temp1, 2);				// divided by 4 coeffs compensation

	for (int e = 0; e < 3; e++) {
		temp2 = mul64(uy, temp1);							// D' * X			Q29 *Q29 = Q58
		temp2 = adds64(temp2, (1 << 28)); // round
		temp2 = RightShift64(temp2, 29);  // -> Q29

		temp2 = subs32(one, temp2);		                // 1 - D' * X

		temp2 = mul64(temp1, temp2);			   // X *( 1 - D' * X)
		temp2 = adds64(temp2, (1 << 28)); // round
		temp2 = RightShift64(temp2, 29);  // -> Q29

		temp2 = adds64(temp1, temp2);          // X + X *( 1 - D' * X)

		temp1 = temp2;                         // new X
	}

	res = mul64(ux, temp2);
	res = adds64(res, (1 << 42));		 // round  

	res = RightShift64(res, 43);		/*res = RightShift32(res, 29);  res = LeftShift32(res, 2);*/

										//Sign Checking
	if (((x > 0) && (y < 0)) || ((x < 0) && (y > 0))) {
		res_sig = res | (1 << 15);
	}
	else {
		res_sig = res;
	}

	return res_sig;
}

int Norm32(uint32_t x) {
	//Returns the number of (leading zeros-1) = number of needed shifts
	//INCREASING THE NUMBER TO THE RANGE (0.5; 1)
	int i = 0;

	if (x == 0) {
		return 0;
	}

	while (x <= (1 << 30))
	{
		x = x << 1;
		i++;
	}

	return i;
}

int32_t div32(int32_t x, int32_t y) // x - Numerator, y - Denumenator
{
	// Newton–Raphson division method
	// for small num / big num
	// for numbers < 1

	uint32_t coef1, coef2, one, ux, uy, temp1;
	uint64_t temp, res, temp3;

	int32_t res_sig;
	int i = 0;

	if (x > y) {
		printf("Error! You can divide just a small number by bigger number!\n");
		system("pause");
		return 0;
	}
	if (y == 0) {
		printf("Error! Division by zero!\n");
		system("pause");
		return 0;
	}
	if (x == y) {
		return INT32MAX;
	}

	ux = (uint32_t)x; // N'
	uy = (uint32_t)y; // D'

	i = Norm32(uy);  
	uy = uy << i;
	ux = ux << i;

	uy = RightShift32(uy, 2);
	ux = RightShift32(ux, 2);

	coef1 = float2fixed(0.4705882352941176);  // Q31 divided by 4		32/17    
	coef2 = float2fixed(0.7058823529411765);  // Q31 divided by 4	    48/17   

	coef1 = RightShift32(coef1, 2);	
	coef2 = RightShift32(coef2, 2);
	one = RightShift32(INT32_MAX, 2);

	//Initial approximation
	temp = mul64(coef1, uy);					// Q29 * Q29 = Q58,
	temp = adds64(temp, (1 << 28)); // round
	temp = RightShift64(temp, 29); // -> Q29
	temp1 = subs32(coef2, (int32_t)temp);		// X in Q29
												// X := 48/17 - 32/17 × D'

	temp1 = LeftShift32(temp1, 2);				// divided by 4 coeffs compensation

	for (int e = 0; e < 3; e++) {
		temp3 = mul64(uy, temp1);							// D' * X			Q29 *Q29 = Q58
		temp3 = adds64(temp3, (1 << 28)); // round
		temp3 = RightShift64(temp3, 29);  // -> Q29

		temp3 = subs32(one, temp3);		                // 1 - D' * X

		temp3 = mul64(temp1, temp3);			   // X *( 1 - D' * X)
		temp3 = adds64(temp3, (1 << 28)); // round
		temp3 = RightShift64(temp3, 29);  // -> Q29

		temp3 = adds64(temp1, temp3);          // X + X *( 1 - D' * X)

		temp1 = temp3;                         // new X
	}

	res = mul64(ux, temp3);
	res = adds64(res, (1 << 26));		 // round

	res = RightShift64(res, 27);		/*res = RightShift32(res, 29);  res = LeftShift32(res, 2);*/

										//Sign Checking
	if (((x > 0) && (y < 0)) || ((x < 0) && (y > 0))) {
		res_sig = res | (1 << 31);
	}
	else {
		res_sig = res;
	}

	return res_sig;
}


// Log and power

int Norm16dec(uint32_t x) {
	//Returns the number of (leading zeros-1) = number of needed shifts
	// DECREASING THE NUMBER TO THE RANGE (0.5; 1)
	// Q15 check
	int i = 0;

	if (x == 0) {
		return 0;
	}

	while (x >= (0x00004000))
	{
		x = x >> 1;
		i++;
	}

	return i;
}

int32_t LinearInterpolation(int16_t x, int32_t num1, int32_t num2) {
	// uint16_t count = RightShift16(x,8) for order number in the table
	// num1,2 in Q27, x in Q15

	uint16_t interpol_bits;
	int16_t int_paramether;
	int32_t int_par27, temp1, temp2, res;
	int64_t temp3, temp4;

	interpol_bits = (uint8_t)x;

	int_paramether = div16(interpol_bits, 0xff);			   // /255;
	int_par27 = LeftShift32((int32_t)int_paramether, 12);   // -> Q27

	temp1 = subs32(((1LL << 27) - 1), int_par27);      // linear interpolation
	temp3 = mul64(temp1, num1);
	temp3 = adds64(temp3, (1 << 26));//round
	temp1 = (int32_t) RightShift64(temp3, 27);			    // -> Q27 

	temp4 = mul64(int_par27, num2);
	temp4 = adds64(temp4, (1 << 26));//round
	temp2 = RightShift64(temp4, 27);						// -> Q27 

	res = adds32(temp1, temp2);

	return res;
}

int32_t log2calc(int32_t x32) {
	// for the all values in Q15 with headroom bits up to int32_t
	// result in Q27
	// i - number of needed shifts

	//table is in Q27
	int32_t log2table[] = {
		 INT32MIN,
		-939524096,
		-805306368,
		-726794030,
		-671088640,
		-627880182,
		-592576302,
		-562727296,
		-536870912,
		-514063964,
		-493662454,
		-475207043,
		-458358574,
		-442859484,
		-428509568,
		-415150116,
		-402653184,
		-390914120,
		-379846236,
		-369376916,
		-359444726,
		-349997230,
		-340989315,
		-332381887,
		-324140846,
		-316236269,
		-308641756,
		-301333898,
		-294291840,
		-287496924,
		-280932388,
		-274583123,
		-268435456,
		-262476978,
		-256696392,
		-251083383,
		-245628508,
		-240323101,
		-235159188,
		-230129418,
		-225226998,
		-220445638,
		-215779502,
		-211223169,
		-206771587,
		-202420050,
		-198164159,
		-193999800,
		-189923118,
		-185930497,
		-182018541,
		-178184054,
		-174424028,
		-170735626,
		-167116170,
		-163563130,
		-160074112,
		-156646850,
		-153279196,
		-149969111,
		-146714660,
		-143514005,
		-140365395,
		-137267165,
		-134217728,
		-131215570,
		-128259250,
		-125347387,
		-122478664,
		-119651821,
		-116865655,
		-114119009,
		-111410780,
		-108739906,
		-106105373,
		-103506203,
		-100941460,
		-98410244,
		-95911690,
		-93444966,
		-91009270,
		-88603832,
		-86227910,
		-83880787,
		-81561774,
		-79270206,
		-77005441,
		-74766858,
		-72553859,
		-70365867,
		-68202322,
		-66062685,
		-63946431,
		-61853057,
		-59782072,
		-57733002,
		-55705390,
		-53698789,
		-51712769,
		-49746912,
		-47800813,
		-45874078,
		-43966326,
		-42077186,
		-40206300,
		-38353317,
		-36517898,
		-34699713,
		-32898442,
		-31113773,
		-29345402,
		-27593035,
		-25856384,
		-24135171,
		-22429122,
		-20737974,
		-19061468,
		-17399352,
		-15751383,
		-14117320,
		-12496932,
		-10889991,
		-9296277,
		-7715572,
		-6147667,
		-4592355,
		-3049437,
		-1518715,
		0
	};

	uint16_t count;
	int16_t x;
	int32_t res, num1, num2;
	int i;

	if (x32 <= 0) {
		printf("log2 can`t be calculated for value <= 0!");
		system("pause");
		return 0;
	}

	i = Norm16dec(x32);
	x = RightShift32(x32, i);

	count = RightShift32(x, 8);

	num1 = log2table[count]; //in Q27
	num2 = log2table[count+1];

	res = LinearInterpolation(x, num1, num2);

	//adding of i for the integer part of log2
	for (; i > 0; i--) {
		res = adds64(res, 0x07ffffff);
	}
	return res;  // -> Q27
}

int NormQ27(int64_t x) {
	// x in Q27 -> to the range (-1,1) with subs
	int i = 0;

	if (x >= 0) {
		while (x >= 0x8000000) {
			x = subs64(x, ((1 << 27)-1));
			i++;
		}
	}
	if (x < 0) {
		while (x <= (-(1 << 27))) {
			x = adds64(x, ((1 << 27)-1));
			i++;
		}
	}
	return i;
}

int32_t Pow(int32_t x32, int32_t y32) {
	// x - in Q15 with 16 bit hr with overflow
	// y - Q27
	// result Q27
	// x^y = 2^ (y* log2 x) formula

	int16_t x15;
	int32_t res32, num1, num2;
	int64_t res64;
	int i = 0;
	int IsPositive = 0;
	uint16_t count;

	//tables in Q27
	int32_t pow2pos[] = {
		134217728,
		134946516,
		135679263,
		136415988,
		137156713,
		137901460,
		138650252,
		139403109,
		140160054,
		140921109,
		141686297,
		142455639,
		143229160,
		144006880,
		144788823,
		145575012,
		146365470,
		147160220,
		147959285,
		148762690,
		149570456,
		150382609,
		151199172,
		152020168,
		152845623,
		153675559,
		154510002,
		155348976,
		156192506,
		157040616,
		157893331,
		158750676,
		159612677,
		160479358,
		161350745,
		162226863,
		163107739,
		163993398,
		164883866,
		165779169,
		166679334,
		167584386,
		168494352,
		169409260,
		170329136,
		171254006,
		172183898,
		173118840,
		174058858,
		175003980,
		175954235,
		176909649,
		177870251,
		178836069,
		179807131,
		180783466,
		181765102,
		182752069,
		183744394,
		184742108,
		185745240,
		186753818,
		187767873,
		188787434,
		189812531,
		190843194,
		191879454,
		192921340,
		193968884,
		195022116,
		196081067,
		197145767,
		198216249,
		199292544,
		200374683,
		201462698,
		202556620,
		203656483,
		204762317,
		205874156,
		206992033,
		208115979,
		209246028,
		210382213,
		211524568,
		212673125,
		213827919,
		214988984,
		216156353,
		217330060,
		218510141,
		219696630,
		220889561,
		222088969,
		223294890,
		224507359,
		225726412,
		226952084,
		228184412,
		229423430,
		230669177,
		231921688,
		233181000,
		234447149,
		235720174,
		237000111,
		238286999,
		239580874,
		240881774,
		242189738,
		243504805,
		244827012,
		246156398,
		247493003,
		248836865,
		250188025,
		251546521,
		252912394,
		254285683,
		255666429,
		257054673,
		258450454,
		259853815,
		261264795,
		262683437,
		264109782,
		265543872,
		266985749,
		268435456
	};

	int32_t pow2neg[] = {
		134217728,
		133492874,
		132771936,
		132054891,
		131341718,
		130632397,
		129926907,
		129225227,
		128527336,
		127833214,
		127142841,
		126456197,
		125773260,
		125094012,
		124418432,
		123746501,
		123078199,
		122413506,
		121752402,
		121094869,
		120440887,
		119790437,
		119143499,
		118500055,
		117860087,
		117223574,
		116590500,
		115960844,
		115334588,
		114711715,
		114092206,
		113476042,
		112863206,
		112253679,
		111647445,
		111044484,
		110444780,
		109848315,
		109255070,
		108665030,
		108078176,
		107494492,
		106913959,
		106336562,
		105762284,
		105191106,
		104623014,
		104057989,
		103496016,
		102937078,
		102381158,
		101828241,
		101278310,
		100731349,
		100187341,
		99646272,
		99108124,
		98572883,
		98040533,
		97511058,
		96984442,
		96460670,
		95939727,
		95421597,
		94906265,
		94393717,
		93883936,
		93376909,
		92872620,
		92371054,
		91872197,
		91376034,
		90882551,
		90391733,
		89903565,
		89418034,
		88935125,
		88454824,
		87977117,
		87501990,
		87029429,
		86559420,
		86091949,
		85627003,
		85164568,
		84704630,
		84247176,
		83792193,
		83339667,
		82889584,
		82441933,
		81996699,
		81553869,
		81113431,
		80675372,
		80239679,
		79806338,
		79375338,
		78946665,
		78520308,
		78096253,
		77674488,
		77255001,
		76837779,
		76422811,
		76010084,
		75599586,
		75191304,
		74785228,
		74381345,
		73979642,
		73580110,
		73182735,
		72787506,
		72394411,
		72003440,
		71614580,
		71227819,
		70843148,
		70460554,
		70080027,
		69701554,
		69325126,
		68950730,
		68578356,
		68207994,
		67839631,
		67473258,
		67108864
	};

	res32 = log2calc(x32);		// Q27

	res64 = mul64(res32, y32);
	res64 = adds64(res64, (1 << 26));				//round
	res32 = RightShift64(res64, 27);			   // -> Q27 power! for 2^res32 calculation

	if (res32 >= 0)
		IsPositive = 1;

	i = NormQ27(res32);   //Normalization
	if ((i > 3) && (y32 > 0)) {
		printf("Power function works just with numbers less than 4.0 (y < 4.0)!\n");
		printf("Please enter higher Ratio or change a Threshold!\n");
		system("pause");
	}

	if (IsPositive) {    // Normalization (tha same subs/adds as in NormQ27 function) 
		for (int q = i; q > 0; q--)
		{
			res32 = subs32(res32, (1 << 27)-1);
		}
	}
	else {
		for (int q = i; q > 0; q--)
		{
			res32 = adds32(res32, (1 << 27)-1);
		}
	}

	// Interpolation
	x15 = RightShift32(res32, 12); //-> Q15
	count = RightShift32(x15, 8);

	if (IsPositive) {
		num1 = pow2pos[count];    //in Q27   
		num2 = pow2pos[count + 1];
	}
	else {
		count = ~count;
		num1 = pow2neg[count];
		num2 = pow2neg[count + 1];
	}

	res32 = LinearInterpolation(x15, num1, num2);  //in Q27 result							 

	if (IsPositive)
		res32 = res32 << i; //needed integer shift
	else
		res32 = res32 >> i;

	return res32;
}

int32_t PowQ15(int32_t x32, int32_t y32) {
	// x - in Q15 with 16 bit hr with overflow
	// y - Q15 + bits of headroom
	// result Q14 with hr bits
	// x^y = 2^ (y* log2 x) formula

	int16_t x15;
	int32_t res32, num1, num2;
	int64_t res64;
	int i = 0;
	int IsPositive = 0;
	uint16_t count;

	//tables in Q27
	int32_t pow2pos[] = {
		134217728,
		134946516,
		135679263,
		136415988,
		137156713,
		137901460,
		138650252,
		139403109,
		140160054,
		140921109,
		141686297,
		142455639,
		143229160,
		144006880,
		144788823,
		145575012,
		146365470,
		147160220,
		147959285,
		148762690,
		149570456,
		150382609,
		151199172,
		152020168,
		152845623,
		153675559,
		154510002,
		155348976,
		156192506,
		157040616,
		157893331,
		158750676,
		159612677,
		160479358,
		161350745,
		162226863,
		163107739,
		163993398,
		164883866,
		165779169,
		166679334,
		167584386,
		168494352,
		169409260,
		170329136,
		171254006,
		172183898,
		173118840,
		174058858,
		175003980,
		175954235,
		176909649,
		177870251,
		178836069,
		179807131,
		180783466,
		181765102,
		182752069,
		183744394,
		184742108,
		185745240,
		186753818,
		187767873,
		188787434,
		189812531,
		190843194,
		191879454,
		192921340,
		193968884,
		195022116,
		196081067,
		197145767,
		198216249,
		199292544,
		200374683,
		201462698,
		202556620,
		203656483,
		204762317,
		205874156,
		206992033,
		208115979,
		209246028,
		210382213,
		211524568,
		212673125,
		213827919,
		214988984,
		216156353,
		217330060,
		218510141,
		219696630,
		220889561,
		222088969,
		223294890,
		224507359,
		225726412,
		226952084,
		228184412,
		229423430,
		230669177,
		231921688,
		233181000,
		234447149,
		235720174,
		237000111,
		238286999,
		239580874,
		240881774,
		242189738,
		243504805,
		244827012,
		246156398,
		247493003,
		248836865,
		250188025,
		251546521,
		252912394,
		254285683,
		255666429,
		257054673,
		258450454,
		259853815,
		261264795,
		262683437,
		264109782,
		265543872,
		266985749,
		268435456
	};

	int32_t pow2neg[] = {
		134217728,
		133492874,
		132771936,
		132054891,
		131341718,
		130632397,
		129926907,
		129225227,
		128527336,
		127833214,
		127142841,
		126456197,
		125773260,
		125094012,
		124418432,
		123746501,
		123078199,
		122413506,
		121752402,
		121094869,
		120440887,
		119790437,
		119143499,
		118500055,
		117860087,
		117223574,
		116590500,
		115960844,
		115334588,
		114711715,
		114092206,
		113476042,
		112863206,
		112253679,
		111647445,
		111044484,
		110444780,
		109848315,
		109255070,
		108665030,
		108078176,
		107494492,
		106913959,
		106336562,
		105762284,
		105191106,
		104623014,
		104057989,
		103496016,
		102937078,
		102381158,
		101828241,
		101278310,
		100731349,
		100187341,
		99646272,
		99108124,
		98572883,
		98040533,
		97511058,
		96984442,
		96460670,
		95939727,
		95421597,
		94906265,
		94393717,
		93883936,
		93376909,
		92872620,
		92371054,
		91872197,
		91376034,
		90882551,
		90391733,
		89903565,
		89418034,
		88935125,
		88454824,
		87977117,
		87501990,
		87029429,
		86559420,
		86091949,
		85627003,
		85164568,
		84704630,
		84247176,
		83792193,
		83339667,
		82889584,
		82441933,
		81996699,
		81553869,
		81113431,
		80675372,
		80239679,
		79806338,
		79375338,
		78946665,
		78520308,
		78096253,
		77674488,
		77255001,
		76837779,
		76422811,
		76010084,
		75599586,
		75191304,
		74785228,
		74381345,
		73979642,
		73580110,
		73182735,
		72787506,
		72394411,
		72003440,
		71614580,
		71227819,
		70843148,
		70460554,
		70080027,
		69701554,
		69325126,
		68950730,
		68578356,
		68207994,
		67839631,
		67473258,
		67108864
	};

	res32 = log2calc(x32);		// Q27
	res32 = RightShift32(res32, 12); //-> Q15

	res64 = mul64(res32, y32);						//Q15 * Q15 = Q30
	res64 = adds64(res64, (1 << 2));				//round
	res64 = RightShift64(res64, 3);					// -> Q27 power! for 2^res32 calculation

	if (res64 >= 0)
		IsPositive = 1;

	i = NormQ27(res64);   //Normalization
						  //if ((i > 3) && (y32 > 0)) {
						  //	printf("Power function works just with numbers less than 4.0 (y < 4.0)!");
						  //	//system("pause");
						  //}

	if (IsPositive) {    // Normalization (tha same subs/adds as in NormQ27 function) 
		for (int q = i; q > 0; q--)
		{
			res64 = subs64(res64, (1 << 27) - 1);
		}
	}
	else {
		for (int q = i; q > 0; q--)
		{
			res64 = adds64(res64, (1 << 27) - 1);
		}
	}

	// Interpolation
	x15 = RightShift64(res64, 12); //-> Q15
	count = RightShift32(x15, 8);

	if (IsPositive) {
		num1 = pow2pos[count];    //in Q27   
		num2 = pow2pos[count + 1];
	}
	else {
		count = ~count;
		num1 = pow2neg[count];
		num2 = pow2neg[count + 1];
	}

	res32 = LinearInterpolation(x15, num1, num2);  //in Q27 result							 
	res32 = RightShift64(res32, 13); //-> Q14

	if (i != 0) {
		if (IsPositive)
			res32 = res32 << i; //needed integer shift
		else
			res32 = res32 >> i;
	}
	return res32; //Q14 result with bits of headroom up to 31
}
