int mode5(char *shmaddr);

// Array for problem set, example set, solution set
char problem[3][10][33] = {
// mode1
	{/*1*/{"KOREA"},/*2*/{"HUNGARY"},/*3*/{"CHINA"},/*4*/{"CANADA"},/*5*/{"BRAZIL"},/*6*/{"JAPAN"},/*7*/{"ALGERIA"},/*8*/{"AUSTRALIA"},/*9*/{"UK"},/*10*/{"GREECE"}},
// mode2
	{/*1*/{"Don’t judge a ( ) by its cover"},/*2*/{"Too many ( ) spoil the broth"},/*3*/{"Many hands make ( ) work"},/*4*/{"There is no place like ( )"},/*5*/{"Easy ( ), easy go"},/*6*/{"( ) is the best policy"},/*7*/{"( ) makes perfect"},/*8*/{"The more, the ( )"},/*9*/{"( ) before you leap"},/*10*/{"The early bird catches the ( )"}},
// mode3
	{/*1*/{"2 + 3"},/*2*/{"3 * 9"},/*3*/{"1 + 2 * 8"},/*4*/{"2 * 2 + 8"},/*5*/{"3 * 2 - 1 * 4"},/*6*/{"4 / 2 * 3 + 2"},/*7*/{"9 * 6 - 8 / 2"},/*8*/{"0 + 3 * 0"},/*9*/{"10 / 2 * 8"},/*10*/{"1 + 2 + 3 + 4 + 5"}}
};

int solution[3][10] = {	// SW1의 값과 직접 비교하기 때문에 1 또는 2로 판단
// mode1 - Capital
	{1,2,2,1,2,2,1,2,1,2},
// mode2 - Sayings
	{1,2,2,1,2,2,1,2,1,1},
// mode3 - Calculation
	{2,1,1,1,1,2,1,2,1,1}
};

char example[3][10][2][20] = {
// mode1 - Capital
	{ /*1*/{{"Seoul"},{"Pusan"}}, /*2*/{{"Kiev"},{"Budapest"}}, /*3*/{{"Oslo"},{"Beijing"}}, /*4*/{{"Ottawa"},{"Rome"}}, /*5*/{{"Rio"},{"Brasilia"}}, /*6*/{{"Kyoto"},{"Tokyo"}}, /*7*/{{"Algiers"},{"Cairo"}}, /*8*/{{"Dili"},{"Canberra"}}, /*9*/{{"London"},{"LA"}}, /*10*/{{"Paris"},{"Athens"}} },
// mode2 - Syaings
	{ /*1*/{{"book"},{"cook"}}, /*2*/{{"books"},{"cooks"}}, /*3*/{{"heavy"},{"light"}}, /*4*/{{"home"},{"house"}}, /*5*/{{"get"},{"come"}}, /*6*/{{"Deceit"},{"Honesty"}}, /*7*/{{"Practice"},{"Rest"}}, /*8*/{{"bitter"},{"better"}}, /*9*/{{"Look"},{"Watch"}}, /*10*/{{"worm"},{"warm"}} },
// mode3 - Calculation
	{ /*1*/{{"4"},{"5"}}, /*2*/{{"27"},{"28"}}, /*3*/{{"17"},{"18"}}, /*4*/{{"12"},{"13"}}, /*5*/{{"2"},{"12"}}, /*6*/{{"7"},{"8"}}, /*7*/{{"50"},{"24"}}, /*8*/{{"9"},{"0"}}, /*9*/{{"40"},{"1"}}, /*10*/{{"15"},{"16"}} }
};


// Dot matrix values for Game mode & O,X sign
unsigned char mode_sign_matrix[5][10] = {
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // mode 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // mode 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // mode 3
	{0x3e,0x7f,0x63,0x63,0x63,0x63,0x63,0x63,0x7f,0x3e}, // sign O
	{0x41,0x41,0x22,0x14,0x08,0x08,0x14,0x22,0x41,0x41}  // sign X
};
