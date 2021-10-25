#include <Windows.h>
# include <Siv3D.hpp> // OpenSiv3D v0.6.2

// 迷路のサイズ WxH
#define CWIDTH  33
#define CHEIGHT 33
// キャラクターの縦横サイズ(正方形)
#define SPSIZE  60

// BUGの個数
constexpr int bugs = 8;

// stopper, bug sleeping time
constexpr int sleeptime   = 400;
constexpr int stoppertime = 400;
// 何フレーム書いたら、timeを1減らすのか？
// 1フレームの目安は、 0.033秒。1秒ならば 30
constexpr int timedown = 512;

// enable/disable debug console
constexpr auto DEBUG  = false;
constexpr auto DEBUG2 = false;
constexpr auto DEBUG3 = false;
constexpr auto DEBUG4 = true;
#define _DEBUG4_

// mapping character num, mask bit.
constexpr auto PATH    = 0;
/// @brief 方向抽出用 bit
constexpr auto DIRMASK = 7;   //  方向抽出用
/// @brief 3/4には、CMASKのみを書く。＊PATHには付けない＊
constexpr auto CMASK   = 0b000010000; // 左上以外でPATH以外にはこのbitを付ける。
constexpr auto CHRDIG  = 32; //32  // 下位4bit(方向3bit+1)は方向
constexpr auto CDMASK  =   ~ 0b11111; // char抽出用のマスク not(31)
constexpr auto WALL    = 1 * CHRDIG; //32
constexpr auto STOPPER = 2 * CHRDIG; //64
constexpr auto MAN     = 4 * CHRDIG; //128
constexpr auto BUG     = 8 * CHRDIG; //256
constexpr auto EATMAN = 512;
constexpr auto EATBUG = 1024;
constexpr auto HUMMAN = 2048;
constexpr auto HUMBUG = 4096;
	// 0: no (path)
	// 1: Wall
	// 32: Wall2 (見えない壁)
	// 2: Stopper
	// 4,5,6,7,8: Man (Stop, U, D, R, L)
	// 9,10,11,12: Bug (U,D,R,L)
	// 13,14,15,16: Man with Hammer
	// 17,18,19,20: Hammer
/*
	考え方
	1. drawmapには、char毎に2x2の領域を取る。
	2. 左上は、charの情報、向き、
	3. 他の3つは、CMASKのみ (16になる。)
*/

// direction of bugs and man.
constexpr auto DIRSTAY  = 0;
constexpr auto DIRSTOP  = 0;
constexpr auto DIRUP    = 1;
constexpr auto DIRDOWN  = 2;
constexpr auto DIRRIGHT = 3;
constexpr auto DIRLEFT  = 4;

class Maze {
public:
	const int width = CWIDTH, height = CHEIGHT;
	int maze[CWIDTH][CHEIGHT];

	Maze() {
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				maze[i][j] = 0;
			}
		}
	}

	// 迷路の作成
	void Generate()
	{
		// 外周
		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				if ((i == 0) || (j == 0) || (i == (width - 1)) || (j == (height - 1))) {
					maze[i][j] = WALL;
				}
				else {
					maze[i][j] = PATH;
				}


		// 棒を立て、倒す
		std::random_device rnd;
		for (int x = 2; x < width - 1; x += 2)
		{
			for (int y = 2; y < height - 1; y += 2)
			{
				maze[x][y] = WALL; // 棒を立てる

			//	if (rnd() % 5 == 0) // 1/5 で倒さない
			//	{
			//	    continue;
			//	}

				// 倒せるまで繰り返す
				int direction = rnd() % 4;
				while (true)
				{
					// 1/4 で壁を止める
					if ((rnd() % 4) == 0)
					{
						break;
					}

					// 1/2 で前と同じ方向
					if ((rnd() % 2) == 0)
					{
						// 1行目のみ上に倒せる
						if (y == 2)
							direction = rnd() % 4;
						else
							direction = rnd() % 3;
					}
					else
					{
						// Console.WriteLine("SameDirection");
					}

					// 棒を倒す方向を決める
					int wallX = x;
					int wallY = y;
					switch (direction)
					{
					case 0: // 右
						wallX++;
						break;
					case 1: // 下
						wallY++;
						break;
					case 2: // 左
						wallX--;
						break;
					case 3: // 上
						wallY--;
						break;
					}
					// 壁じゃない場合のみ倒して終了
					if (maze[wallX][wallY] != WALL)
					{
						maze[wallX][wallY] = WALL;
						break;
					}
				}
			}
		}

		// special 出入口の前の壁を開けておく
		//maze[(width - 1) / 2][0] = Path; // Exit
		maze[(width - 1) / 2][1] = PATH; // Exit -1
		//maze[(width - 1) / 2][2] = PATH; // Exit -1 // for Debuging Print;
		maze[(width - 1) / 2][height - 2] = PATH; // Enter -1
		maze[(width - 1) / 2][height - 1] = 3;// Enter

		// for (int i = 0; i < width; i++) {
		// 	String s;
		// 	for (int j = 0; j < width; j++) {
		// 		if (maze[j][i] == WALL) {
		// 			s += U"╋";
		// 		}
		// 		else {
		// 			s += U" ";
		// 		}
		// 	}
		// 	Console << s;
		// }

	}

};

// ゲーム中 (D3D の
struct GameData {
	int aliens;	// number of aliens
	int score;	// score;
	int men;	// number of men
	int screen; // 面数
	int time;
	Maze maze;
	double spawnTime; // 1/Framerate;
	int eaten;
};
using App = SceneManager<String, GameData>;

// grobal variable.
struct Grobal {
	int highscore = 0;
	boolean ingame = false;
	Stopwatch stopwatch;
	boolean enableSound;
} global;


/*
static class InGameSound {
public:
	int enableSound;

	InGameSound(boolean enable)
	{
		enableSound = enable;
	}
	void playForward()
	{
		if (enableSound) {
			return;
		}
		;
	}
	void playHammer()
	{
		if (enableSound) {
			return;
		}
		;
	}
	void playDeath()
	{
		if (enableSound) {
			return;
		}
		;
	}
	void playSuccess()
	{
		if (enableSound) {
			return;
		}
		;
	}
};
*/

// Title Scene
class Title : public App::Scene
{
private:
	int state = 0;
	int counts;
	double framerate = 0;
public:
	int SoundState = -1; // 1:EnableSound, 0:Mute Sound
	Title(const InitData& init)
		: IScene{ init }
	{
		SoundState = -1;
		counts= 0;
		framerate = 0;
	}

	~Title()
	{
		// 計測したframerateを設定
		getData().spawnTime = framerate * 2.05;
	}

	void draw() const override
	{
		//	for (s3d::String m : Meeeage) {
		//	}

	}

	void update() override {
		// Title でframerateを計測して、ゲーム開始時に2.05倍で設定する。
		// 極端に遅い場合の対策だけど、極端に速い場合の対策はしていない
		//■■■■■■■■■■■■■■■■■■■■■■ ■■■■■■■■■■■■■■■■■■■■■■ ■■■■■■■■■■■■■■■■■■■■■■
		if (counts < 100) {
			counts += 1;
			double t = Scene::DeltaTime();
			framerate = (framerate * counts + t) / (counts + 1);
		}
		const s3d::String hoge[] = {
		U"",
		U"　　　＊＊＊＊＊＊　ＢＵＧ　ＦＩＲＥ　＊＊＊＊＊＊",
		U"",
		U"　　　　　キー　ノ　セツメイ",
		U"",
		U"　　　　　　　［８］",
		U"　　　　　　　　↑ ",
		U"　　　　　　　　　　　　　　　　　　　　　［Ｘ］・・・・・ストッハ゜ー",
		U"　　［４］← 　　　　　 →［６］",
		U"　　　　　　　　　　　　　　　　　　　　　＜ＳＨＩＦＴ＞・・ハンマー",
		U"　　　　　　　　↓ ",
		U"　　　　　　　［２］",
		U"",
		U"オト　ヲ　ケシマスカ？　（ｙ／ｎ）",
		};
		//Print << U"Title::Title()";
		int ly = 0, lx = 20;
		//const Font font{22, U"Font/DotGothic16-Regular.ttf"};
		//const Font font{22};

		//font(U"SCORE").draw(300, 300);
		for (s3d::String m : hoge) {
			//	font(U"SCORE").draw(300, 300);
			FontAsset(U"PC8001")(m).draw(lx, ly, Palette::Lightgreen);
			ly += 24;
		}
		boolean yes, no;
		yes = no = false;
		if (SoundState == -1) {
			// 質問キーの時は、upで取得すると、後の処理に影響がない。
			yes = KeyY.up();
			no = KeyN.up();

			if (yes) {
				SoundState = 1;
			}
			if (no) {
				SoundState = 0;
				//while (!KeyN.up()) {};
			}
			if (SoundState == -1) {
				return;
			}
		}
		ly += 24 * 2;

		yes = no = false;
		const s3d::String Setumei = U"　　　　セツメイ　カ゛　イリマスカ（ｙ／ｎ）";
		FontAsset(U"PC8001")(Setumei).draw(lx, ly, Palette::Lightgreen);
		yes = KeyY.down();
		no = KeyN.down();


		if (yes) {
			changeScene(U"Instruction", 0s);
		}
		if (no) {
			changeScene(U"InitGame", 0s);
		}

		//	if (MouseL.down()) {
		//		if (state == 0) {
		//			changeScene(U"Game");
		//			//changeScene(U"Title2");
		//		}
		//		else {
		//			// changeScene(U"Game");
		//		}
		//	}
	}

};

class Instruction :public App::Scene
{
public:
	const s3d::String message[19] = {
			U" ",
			U" ",
			U"　　＜ケ゛ーム　ノ　セツメイ＞",
			U"ケ゛ーム　＜　Ｂ　Ｕ　Ｇ　　Ｆ　Ｉ　Ｒ　Ｅ　＞　ノ　セツメイ　ヲ　イタシ",
			U"マス。",
			U" ",
			U"アナタ　ハ　セイケ゛ンシ゛カンナイ　ニ　メイロ　ヲ　タ゛ッシュツ　シナケ",
			U"レハ゛　ナリマセン。",
			U" ",
			U"シカシ　ソノタメニハ　メイロ　ノ　ナカ　ヲ　ハシリマワル　ＡＬＩＥＮ　＝",
			U"　＜ＢＵＧ＞　　ヲ　スヘ゛テ　ヤッツケネハ゛　ナリマセン。",
			U" ",
			U"ハ゛ク゛　ハ　アナタ　ノ　オイタ　｛ＳＴＯＰＰＥＲ｝　ニアタルト",
			U"シハ゛ラク　ウコ゛カナク　ナリマス。",
			U" ",
			U"ソコヲ　ハンマー　テ゛　タタキツフ゛シテ　クタ゛サイ。",
			U" ",
			U" ",
			U"ワカリマシタカ？　（ｙ／ｎ）■",
	};
	Instruction(const InitData& init)
		: IScene{ init }
	{
	}

	~Instruction()
	{

	}

	void update() override
	{
		int ly = 12, lx = 3;
		//const Font font{22, U"Font/DotGothic16-Regular.ttf"};
		//const Font font{22};

		//font(U"SCORE").draw(300, 300);
		for (s3d::String m : message) {
			//	font(U"SCORE").draw(300, 300);
			FontAsset(U"PC8001")(m).draw(lx, ly);
			ly += 24;
		}
		boolean yes, no;
		yes = KeyY.up();
		no = KeyN.up();

		if (yes) {
			// changeScene(U"Inform");
			changeScene(U"InitGame");
		}

	}

	void draw() const override
	{
	}

};

class Mstring
{
public:
#include <format>
	/// @brief S 数値から、全角文字にする。マイナスには未対応
	/// @param m: 数値、digit:けたすう
	/// @return
	static String itoz(int m, int digit = 0, String sup=U"０")
	{
		String a;
		uint32 t;
		String s;
		a = U"{}"_fmt(m);
		for (int i = 0; i < a.length(); i++)
		{
			t = a[i];
			if (t == 0x0030) s += U"０";  // 0x30 ==='0'
			if (t == 0x0031) s += U"１";
			if (t == 0x0032) s += U"２";
			if (t == 0x0033) s += U"３";
			if (t == 0x0034) s += U"４";
			if (t == 0x0035) s += U"５";
			if (t == 0x0036) s += U"６";
			if (t == 0x0037) s += U"７";
			if (t == 0x0038) s += U"８";
			if (t == 0x0039) s += U"９";
			if (t == 0x002d) s += U"－";
			if (t == 0x002b) s += U"＋";
		}
		if (digit > 0) {
			for (int i = 0; i < digit-a.length(); i++) {
				s = sup + s;
			}
		}
		return s;
	}
};

//GameOver
class GameOver :public App::Scene
{
public:
	boolean highscoreflag;
	const String message[21] = {
		U"",
		U"　　＊＊＊＊＊　Ｇ　Ａ　Ｍ　Ｅ　　Ｏ　Ｖ　Ｅ　Ｒ　＊＊＊＊＊",
		U"",
		U"",
		U"",
		U"",
		U"",
		U"　　　アナタ　ノ　スコア　ハ　{}　テン　テ゛シタ。"_fmt(getData().score),
		U"",
		U"",
		U"",
		U"",
		U"　　　アナタ　ノ　タッシタ　メンスウ　ハ　　{}　　テ゛シタ。"_fmt(getData().screen),
		U"",
		U"",
		U"　　　オツカレサマ　テ゛シタ。",
		U"",
		U"",
		U"　　　　　　　　ｂｙ　Ｙ．ＯＧＩ　（１９８０．１２．２３）",
		U"",
		U"　　　　ＲＥＰＬＡＹ　（　スヘ゜ース　ハ゛ー　）",
	};
	GameOver(const InitData& init)
		: IScene{ init }
	{
		Print << U"GameOver";
		highscoreflag = false;
		if (getData().score > global.highscore) {
			global.highscore = getData().score;
			highscoreflag = true;
		}
		else {
		}

	}

	~GameOver()
	{

	}

	void update() override
	{
		boolean space;
		space = KeySpace.up();

		if (space) {
			// changeScene(U"Inform");
			changeScene(U"InitGame");
		}
	}

	void draw() const override
	{
		int ly = 12, lx = 3;
		//const Font font{22, U"Font/DotGothic16-Regular.ttf"};
		//const Font font{22};
		//font(U"SCORE").draw(300, 300);
		for (s3d::String m : message) {
			//	font(U"SCORE").draw(300, 300);
			FontAsset(U"PC8001")(m).draw(lx, ly);
			ly += 24;
		}
		if (highscoreflag) {
			FontAsset(U"PC8001")(U"　　　＊（コレ　ハ　ハイスコア　テ゛ス。）＊").draw(3, 24 * 8 + 12);  // [9]
		}
	}

};

// ゲーム開始
class InitGame : public App::Scene
{
public:
	InitGame(const InitData& init)
		: IScene{ init }
	{
		global.stopwatch.start();
		getData().score = 0;
		getData().men = 3;
		getData().screen = 0;


	};

	~InitGame()
	{
		if (DEBUG) Print << global.stopwatch.ms() << U"msec";

	}
	void update() override
	{
		if (DEBUG) Print << U"Scene: InitGame";
		changeScene(U"InitScreen", 0s);
	}
	void draw() const override
	{
	}
};

// 新しい面開始
class InitScreen : public App::Scene
{
public:
	InitScreen(const InitData& init)
		: IScene{ init }
	{
		if (DEBUG) Print << U"Scene: InitScreen";
		//面数の更新
		getData().screen += 1;
		//BUG数の更新
		getData().aliens = bugs;
		getData().time = 99;
		//迷路の初期化
		getData().maze.Generate();
	}

	~InitScreen()
	{
		if (DEBUG) Print << global.stopwatch.ms() << U"msec";

	}
	void update() override
	{
		changeScene(U"StartGame", 0s);
	}
	void draw() const override
	{
	}
};
// 面開始
class StartGame : public App::Scene
{
public:
	s3d::String message[8];
	double ti;
	StartGame(const InitData& init)
		: IScene{ init }
	{
		ti = 0;
		if (DEBUG) Print << U"StartGame:";
		// ■■■■■■■■■■■■■■■■未実装
		// 中間報告の表示
		message[0] = U"　　　　　　　　　　　 チュウカンホウコク";
		message[1] = U"　　　　　タ゛イ　{}　メン"_fmt(Mstring::itoz(getData().screen,2));
		message[2] = U"　　　　　メイロ　ノ　レヘ゛ル　　{}"_fmt(Mstring::itoz(99,2));
		message[3] = U"　　　　　モチシ゛カン　{}"_fmt(Mstring::itoz(getData().time, 2));
		message[4] = U"　　　　　フ゜レイヤー　ニンス゛ウ　{}"_fmt(Mstring::itoz(getData().men));
		message[5] = U"　　　　　ＳＣＯＲＥ　　{}"_fmt(Mstring::itoz(getData().score, 6));
		message[6] = U"　　　　　ＨＩＧＨ　ＳＣＯＲＥ　{}"_fmt(Mstring::itoz(global.highscore, 6));
		message[7] = U"　　　　　Ｆ　Ｉ　Ｇ　Ｈ　Ｔ　！！！！！！！";
	}

	~StartGame()
	{
		if (DEBUG) Print << global.stopwatch.ms() << U"msec";
	}
	void update() override
	{
		// 数秒待つ
		double d = Scene::DeltaTime();
		ti = ti + d;
		// 次(10秒後)
		if (ti >= 3) changeScene(U"Game", 0);
	}
	void draw() const override
	{
		int x = 24, y = 24;
		for (int i = 0; i < 8; i++) {
			FontAsset(U"PC8001")(message[i]).draw(x, y);
			y += 48;
		}
	}
};

class Map4draw {
public:
	int map[CWIDTH * 2][CHEIGHT * 2 + 2];
	int width;
	int height;
	void init(Maze maze) {
		height = CHEIGHT * 2 + 2;
		width = CWIDTH * 2;
		for (int xx = 0; xx < CWIDTH; xx++) {
			for (int yy = 0; yy < CHEIGHT; yy++) {
				map[xx * 2][yy * 2] = maze.maze[xx][yy]; // 左上
				if (maze.maze[xx][yy] == PATH) {  // PATHの場合
					map[xx * 2 + 1][yy * 2]     = maze.maze[xx][yy]; // 右上
					map[xx * 2    ][yy * 2 + 1] = maze.maze[xx][yy]; // 左下
					map[xx * 2 + 1][yy * 2 + 1] = maze.maze[xx][yy]; // 右上
				}
				else { // PATHでない場合。(実質WALL)
					map[xx * 2 + 1][yy * 2]     = maze.maze[xx][yy] | CMASK; // 右上
					map[xx * 2    ][yy * 2 + 1] = maze.maze[xx][yy] | CMASK; // 左下
					map[xx * 2 + 1][yy * 2 + 1] = maze.maze[xx][yy] | CMASK; // 右上
				}
			}
		}
		//最下行
		for (int i = 0; i < width; i++) {
			map[i][height - 2] = WALL;
			map[i][height - 1] = WALL;
		}
		height = CHEIGHT * 2 + 2;
		width = CWIDTH * 2;
	}
};


class Game :public App::Scene
{

public:
#ifdef _DEBUG4_
	const Font Debugfont{ 10 };
#endif
	double time;

	//int map[CWIDTH * 2][CHEIGHT * 2]; // mapping
	Map4draw drawmap; // classに変更


//private:

	class Vector {
	public:
		int x, y, dx, dy;
		Vector() {
			x = y = dx = dy = 0;
		}

		void next(int dir)
		{
			switch (dir)
			{
			case DIRSTAY:
				break;
			case DIRUP:
				x = 0; dx = 0;
				y = -1; dy = -1;
				break;
			case DIRDOWN:
				x = 0; dx = 0;
				y = 2; dy = 1;
				break;
			case DIRRIGHT:
				x = 2; dx = 1;
				y = 0; dy = 0;
				break;
			case DIRLEFT:
				x = -1; dx = -1;
				y = 0; dy = 0;
				break;
			default:
				break;
			}
		}

		void prev(int dir)
		{
			switch (dir)
			{
			case DIRSTAY:
				break;
			case DIRUP:
				y = 2; dy = 1;
				break;
			case DIRDOWN:
				y = -1; dy = -1;
				break;
			case DIRRIGHT:
				x = -1; dx = -1;
				break;
			case DIRLEFT:
				x = 2; dx = 1;
				break;
			default:
				break;
			}
		}
	};

	class Char {
	public:
		int x, y;
		int character; //bug, stopperとか
		boolean live; // 存在するかどうか(bugはデフォルトでt, stopperはｆ)

		Char() { // 引数なしのコンストラクタを必ず作成すること。
			x = -1; // -1 : not on screen;
			y = -1;
			live = true;
		}
		Char(boolean l) {
			x = -1; // -1 : not on screen;
			y = -1;
			live = l;
		}
		~Char()
		{

		}

		void draw(Map4draw& drawmap, int dir = 0) // mapに描画
		{
			if (drawmap.map[x][y] != 0) {
				if (DEBUG)Console << U"Draw not 0" << U" X" << x << U" Y" << y
					<< U", ch:" << drawmap.map[x][y];
			}
			// 実際の画面への描写時は左上の情報だけで描画させるため、
			// 左上以外は、bitを立てる。
			drawmap.map[x    ][y    ] = character + dir;
			drawmap.map[x + 1][y    ] = CMASK;
			drawmap.map[x    ][y + 1] = CMASK;
			drawmap.map[x + 1][y + 1] = CMASK;
		}
		void erase(Map4draw& drawmap) // mapに描画
		{
			// // c が、PATHであれば、右下以外も、PATH, そうでなければ、(c | CMASK)
			// for (int i = 0; i < 2; i++) {
			// 	for (int j = 0; i < 2; i++) {
			// 		drawmap.map[x + i][y + j] = 0;
			// 	}
			// }
			drawmap.map[x    ][y    ] = 0;
			drawmap.map[x + 1][y    ] = 0;
			drawmap.map[x    ][y + 1] = 0;
			drawmap.map[x + 1][y + 1] = 0;
		}
	};


	// 各キャラクターのクラス
	// Charクラスから継承

	// d3dのシーンで共有する必要の無い共有する変数
	//Maze maze;	// maze
	int cx, cy;		// man position
	int nextAct;	// man next direction
	int lastDir;	// man last direction (for stopper);
	Vector cv;		// man vector;

	struct Eat {
		int bugx;
		int bugy;
		int bugd;
		int manx;
		int many;
	} eat;
	Stopwatch timer;

	boolean frame1st;
	boolean	keyinfixed;
	boolean hammerflag, stopperflag;


	/// @brief PlayerState
	enum Playerstate {
		NORMAL, DEAD, EXIT,
	};
	Playerstate playerstate;

	int framecount;

	class CStopper : public Char
	{
	public:
		int lifetime;

		CStopper()
		{
			live = false;
			lifetime = 1000; // 1000描画だから、10秒と少しくらい？
			character = STOPPER;
		}

		void init()
		{
			CStopper();
		}

		void put(Map4draw& drawmap, int cx, int cy, int dir)
		{
			if (dir == DIRSTAY) return;

			Vector v;
			v.prev(dir);

			if ((v.dx == 0) && (v.dy == 0)) return;

			int lx = (cx + v.x) & 254;
			int ly = (cy + v.y) & 254;
			if ((lx == cx) && (cy == ly)) return;

			if (drawmap.map[lx][ly] == PATH) {
				x = lx;
				y = ly;
				draw(drawmap);
				live = true;
				lifetime = stoppertime;
			}
			// Print << U"Put X:" << lx << U"Y:" << ly;
			// DateTime date = DateTime::Now();
			// Print << U"PutTime:" << date.minute << U":" << date.second << U"." << date.milliseconds;

		}
		void remove(Map4draw& drawmap)
		{
			if (drawmap.map[x][y] == STOPPER) {
				erase(drawmap);
			}
			live = false;

			// Print << U"Del X:" << x << U"Y:" << y;
			// DateTime date = DateTime::Now();
			// Print << U"DelTime:" << date.minute << U":" << date.second << U"." << date.milliseconds;
		}
		void doing(Map4draw& drawmap)
		{
			lifetime -= 1;
			if (lifetime <= 0) {
				remove(drawmap);  // ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
			}
		}
	};

	class CBug : public Char
	{
	public:
		int direction; // ST,UP,DW, RT,LT
		int sleep;
		int px, py;
		Vector v;
		CBug()
		{
			live      = true;
			character = BUG;
			// default Location;
			x         = CWIDTH - 1;
			y         = 2;
			px = x;
			py = y;
			direction = 0;
			sleep     = 0;
		}

		void init()
		{
			CBug();
		}

		void remove(Map4draw& drawmap)
		{
			live = false;
			remove(drawmap);
		}

		// 座標: 偶数→奇数 方向も決める。
		void odd_doing(Map4draw& drawmap,int cx, int cy)
		{
			// 方向を決める。
			std::random_device rnd;
			int d;
			String s = U"";

			// exit in sleeping;
			if (sleep > 0) {
				direction = 0;
				sleep--;
				return;
			}

			v.dx = v.dy = 0;

			if (DEBUG) { // OK/NG, Location
				if ((x % 2 == 0) || (y % 2 == 0)) {
					Console << U"Odd OK X:" << x << U" Y:" << y;
				}
				else {
					Console << U"Odd NG X:" << x << U" Y:" << y;
				}
			}

			if (DEBUG) { // Display arounds;
				for (int i = y - 6; i < y + 6; i++) {
					s = U"";
					for (int j = x - 6; j < x + 6; j++) {
						if ((j >= 0) && (j < drawmap.width)
							&& (i >= 0) && (i < drawmap.height)) {
							d = drawmap.map[j][i];
							//d = d & ~512;
							//d = d & ~16;
						}
						else {
							d = -1;
						}
						if ((j == x) && (i == y)) {
							s = s + U"*{:>3}, "_fmt(d);
						}
						else {
							if ((d & CMASK) != 0) {
								s = s + U"({:>3}),"_fmt(d);
							}
							else {
								s = s + U"{:>4}, "_fmt(d);
							}
						}
						if ((j < 0) ? -j : j % 2 == 1) s += U" | ";
					}
					Console << s;
					if (((i < 0) ? -i : i % 2) == 1) {
						Console << U"----";
					}
				}
			}

			// まずは自分を消す。
			erase(drawmap);

			// 動ける方向を格納
			Array <int> canmove;

			for (int i = 1; i < 5; i++) {
				v.next(i);

				int x1 = x + v.dx;
				int x2 = x + v.dx * 2;
				int y1 = y + v.dy;
				int y2 = y + v.dy * 2;

				if ((x1 > 0) && (x1 < drawmap.width)
				 && (y1 > 0) && (y1 < drawmap.height)
				 && (x2 > 0) && (x2 < drawmap.width)
				 && (y2 > 0) && (y2 < drawmap.height)) {
					// map の範囲内 でないと、アクセス不可のため、事前に判定
					if (((drawmap.map[x1][y1] & CDMASK) == PATH)
						&& ((drawmap.map[x2][y2] & CDMASK) == PATH)) {
						canmove << i;
						if (DEBUG)Console << U"CanMoveFound" << Format(i);
					}
				}
			}
			if (DEBUG) {
				String st = U"";
				Console << U"Moving direction:" << Format(canmove.size()) << U"Counts";
				for (int i = 0; i < canmove.size(); i++) {
					st = st + Format(canmove[i]);
				}
				Console << U"CanMove:" << st << U"<";
			}

			if (canmove.size() == 0) {
				d = 0;
			}
			else {
				// 前と同じ方向に動けるのか？
				boolean canmoveprevious = false;
				for (int i = 0; i < canmove.size(); i++) {
					if (canmove[i] == direction) canmoveprevious = true;
				}

				//動けた場合、1/n の確率で同じ方向
				if ((canmoveprevious) && ((rnd() % 5) >= 1)) {
					d = direction;
				}
				// else { // 1/n の確率で、反対方向
				// }
				else { //そうでなければ、ランダム
					d = canmove[rnd() % canmove.size()];
				}
			}

			direction = d;
			v.next(direction);

			if (DEBUG) {
				Console << U"Selected dir:" << d;
				Console << U"X:" << x << U" ->" << x + v.dx << U" dx:" << v.dx;
				Console << U"Y:" << y << U" ->" << y + v.dy << U" dy:" << v.dy;
			}

			if (direction != 0) {
				// 半分動かす。(中途半端な状態に
				px = x;
				py = y;
				x = x + v.dx;
				y = y + v.dy;
				if (x <= 2) x = 2;
				if (y <= 2) y = 2;
				if (x >= drawmap.width - 4) x = drawmap.width - 4;
				if (y >= drawmap.height - 4) y = drawmap.height - 4;
			}
			draw(drawmap, d);
		}



		/// @brief 奇数→偶数
		/// @param drawmap
		int even_doing(Map4draw& drawmap, int cx, int cy)
		{
			// 半分動かす。
			// bug同士のduplicateは許す。

			if (sleep > 0) direction = 0;

			// 方向が定まっていないのならば、そのままで、return;
			if (direction == 0) {
				return 0;
			}

			if (DEBUG) Console << U"MoveDir:" << direction;
			//v.next(direction); // ■■■■■■■■■なぜかdx,dyがおかしい。

			if (DEBUG) {
				Console << U"X:" << x << U"->" << x + v.dx << U" dx:" << v.dx;
				Console << U"Y:" << y << U"->" << y + v.dy << U" dy:" << v.dy;
			}

			// なぜか、演算後に奇数になるのは、return
			if (((x + v.dx) % 2 != 0) || ((y + v.dy) % 2 != 0)) {
				// ■■■■■■■■■■■■■■■■■ 無理矢理 奇数にしたけど、大丈夫？
				erase(drawmap);
				if ((x + v.dx) % 2 != 0) {
					x = x + v.dx - 1;
				}
				if ((y + v.dy) % 2 != 0) {
					y = y + v.dy - 1;
				}
				draw(drawmap, direction);
				if (DEBUG) {
					Console << U"Corrected: X:" << x << U" Y:" << y;
				}
				return 0;
			}

			//けす
			erase(drawmap);

			x += v.dx;
			y += v.dy;

			if (x <= 2) x = 2;
			if (y <= 2) y = 2;
			if (x >= drawmap.width - 4) x = drawmap.width - 4;
			if (y >= drawmap.height - 4) y = drawmap.height - 4;

			draw(drawmap, direction);

			if ((x % 2 != 0) || (y % 2 != 0)) {
				Console << U"Even NG X:" << x << U" Y:" << y;
			}

			// Check stopper;
			v.next(direction);
			int x1 = x + v.dx;
			int x2 = x + v.dx * 2;
			int y1 = y + v.dy;
			int y2 = y + v.dy * 2;

			// sleep の対応
			if ((x1 > 0) && (x1 < drawmap.width)
			 && (y1 > 0) && (y1 < drawmap.height)
			 && (x2 > 0) && (x2 < drawmap.width)
			 && (y2 > 0) && (y2 < drawmap.height)) {
				// map の範囲内 でないと、アクセス不可のため、事前に判定
				if (((drawmap.map[x1][y1] & CDMASK) == STOPPER)
					|| ((drawmap.map[x2][y2] & CDMASK) == STOPPER)) {
					sleep = sleeptime;
				}
			}

			// Eat
			if ((x1 > 0) && (x1 < drawmap.width)
			 && (y1 > 0) && (y1 < drawmap.height)
			 && (x2 > 0) && (x2 < drawmap.width)
			 && (y2 > 0) && (y2 < drawmap.height)) {
				// map の範囲内 でないと、アクセス不可のため、事前に判定
				if (((x1 == cx) && (y1 == cy))
				   || ((x2 == cx) && (y2 == cy))) {
					if (DEBUG4) Print << U"Eaten";
					return 1; // Dead
				}
			}
			return 0;
		}
	};

	// Characters...
	CStopper cstopper[8];
	CBug cbug[8];

	// Texture Define..
	const Texture bug[4][2] = {  // Bug Direction UDRL x2 (Animation)
		{ Texture(U"img/BugU1.png"), Texture(U"img/BugU2.png")},
		{ Texture(U"img/BugD1.png"), Texture(U"img/BugD2.png")},
		{ Texture(U"img/BugR1.png"), Texture(U"img/BugR2.png")},
		{ Texture(U"img/BugL1.png"), Texture(U"img/BugL2.png")},
	};

	// Texture Define..
	const Texture eaten[4][2] = {  // Bug , man
		{ Texture(U"img/EatBU.png"), Texture(U"img/EatMU.png")},
		{ Texture(U"img/EatBD.png"), Texture(U"img/EatMD.png")},
		{ Texture(U"img/EatBR.png"), Texture(U"img/EatMR.png")},
		{ Texture(U"img/EatBL.png"), Texture(U"img/EatML.png")},
	};

	const Texture man[5][2] = {  // Man Direction UDRLS x2(Animation)
		{ Texture(U"img/ManSt.png"), Texture(U"img/ManSt.png")},
		{ Texture(U"img/ManU1.png"), Texture(U"img/ManU2.png")},
		{ Texture(U"img/ManD1.png"), Texture(U"img/ManD2.png")},
		{ Texture(U"img/ManR1.png"), Texture(U"img/ManR2.png")},
		{ Texture(U"img/ManL1.png"), Texture(U"img/ManL2.png")},
	};

	const Texture hman[4][2] = {  // Man + Hammer Direction UDRL x2(Man + Hammer)
		{ Texture(U"img/ManU1.png"), Texture(U"img/ManU2.png")},
		{ Texture(U"img/ManD1.png"), Texture(U"img/ManD2.png")},
		{ Texture(U"img/ManR1.png"), Texture(U"img/ManR2.png")},
		{ Texture(U"img/ManL1.png"), Texture(U"img/ManL2.png")},
	};
	//Texture wall;
	const Texture wall = Texture(U"img/Wall.png");
	const Texture stopper = Texture(U"img/Stopper.png");
	const Texture path = Texture(U"img/PATH.png");

	Game(const InitData& init)
		:IScene{ init }
	{
		drawmap.init(getData().maze);
		// start location 32,64 (16,32)
		cx = (CWIDTH - 1);
		cy = (CHEIGHT - 1) * 2;
		nextAct = 0;
		lastDir = 0;
		cv;

		eat.bugx = eat.bugy = eat.bugd = 0;
		eat.manx = eat.many = 0;

		accumulator = 0;

		frame1st = true;

		keyinfixed = false;

		hammerflag = false;
		stopperflag = false;

		playerstate = NORMAL;

		for (int i = 0; i < 8; i++) {
			cstopper[i].init();
			cbug[i].init();
		}

		framecount = 0;

		if (DEBUG) {
			Vector v;
			Console << U"VectorCheck:";
			for (int i = 0; i < 6; i++) {
				v.next(i);
				Console << U"Dir:" << Format(i) << U" dx:" << Format(v.dx) << U" dy:" << Format(v.dy);
			}
		}

		if (DEBUG2)Console << U"Start!";
		global.ingame = true;

		getData().eaten = 0;
		double d = getData().spawnTime;
		if (d > 0) {
			if (d > 0.03) {  // 60Hz (0.03333)より速いレートは設定しない。
				spawnTime = getData().spawnTime;
			}
		}
	}

public:
	double spawnTime = 0.03333; // Moving frame [s]
	double framspersec = 5;
	double accumulator;

	void update() override
	{
		//debug showing map info in Console...
		if constexpr (DEBUG2) if (KeySpace.pressed()) {
			String s;
			Console << U"cx:{} cy:{} Frame:{}"_fmt(cx, cy, frame1st);

			for (int yy = cy - 12; yy <= cy + 12; yy++) {
				s = U"";
				for (int xx = cx - 12; xx <= cx + 12; xx++) {
					if ((cx >= 0) && (cx <= drawmap.width) && (cy >= 0) && (cy <= drawmap.height)) {
						if ((xx == cx) && (yy == cy)) {
							s = s + U"*{:>2},"_fmt(drawmap.map[xx][yy]);
						}
						else {
							s = s + U"{:>3},"_fmt(drawmap.map[xx][yy]);
						}
					}
					else {
						s = s + U"xxx ";
					}
					if (Abs(xx) % 2 == 1) {
						s = s + U" | ";
					}
				}
				Console << s;
				if (Abs(yy) % 2 == 1) {
					Console << U"------";
				}
			}
			return;
		}

		++framecount;
		if (framecount == timedown) {
			getData().time = getData().time - 1;;
			framecount = 0;
		}


		//
		accumulator += Scene::DeltaTime();
		if (playerstate == DEAD) {
			// Dead!!
			if (timer.isRunning()) {
#ifndef _DEBUG4_
				if (timer >= 3s) {//stop erase
#else
				if (timer >= 3s) {//stop erase
#endif
					int p = (getData().men -= 1);
					if (p <= 0) {
						changeScene(U"GameOver", 0s);
					}
					changeScene(U"StartGame", 0s);
				}
				else {
					// nothing to do.
				}
			}
			else {// Timer Start and Draw death.
				timer.start();
				drawmap.map[eat.bugx][eat.bugy] = EATBUG | eat.bugd;
				drawmap.map[eat.manx][eat.many] = EATMAN | eat.bugd;
			}
		}
		else {
			//keyinfixed = true;
			if (!keyinfixed) {
				boolean
					upKey = (KeyUp | KeyNum8).pressed(),
					dnKey = (KeyDown | KeyNum2).pressed(),
					rtKey = (KeyRight | KeyNum6).pressed(),
					lfKey = (KeyLeft | KeyNum4).pressed(),
					hmKey = (KeyShift).pressed(),
					stKey = (KeyX).pressed();
				//	Print << U"KeyIN:" << upKey << dnKey << rtKey << lfKey << hmKey << stKey;
				if ((cx % 2 == 0) && (cy % 2 == 0)) {
					if (lfKey) {
						nextAct = DIRLEFT;
					}
					else if (rtKey) {
						nextAct = DIRRIGHT;
					}
					else if (dnKey) {
						nextAct = DIRDOWN;
					}
					else if (upKey) {
						nextAct = DIRUP;
					}

					// 方向入力が無く、keyinが確定していなければ、方向を0にリセットする。
					if (!upKey && !dnKey && !rtKey && !lfKey && !keyinfixed) {
						nextAct = 0;
					}
					else {
						Vector v2;
						v2.next(nextAct);
						int xx = (cx + v2.x) & 254;
						int yy = (cy + v2.y) % 254;
						if (((drawmap.map[xx][yy] & CDMASK)== WALL)
						|| ((drawmap.map[xx][yy] & CDMASK)== STOPPER)) {
							nextAct = 0;
						}
					}

					if (stKey) stopperflag = true;
					if (hmKey) hammerflag = true;
				}
			}

			if (accumulator <= spawnTime) {

				//if (nextAct != 0)lastDir = nextAct; // ここだと、STOPPERはいろいろな方向に出せる。
				if (!canMove(cx, cy, nextAct)) {
					nextAct = 0;
				}

				if (nextAct != 0)lastDir = nextAct; // ここだと、STPPERは来た方向にしか出せない。

				if (frame1st) {
					// 奇数描画で行う処理
					frame1st = false;
					//keyinfixed = true;

					//Print << U"Do1:LastState:" << framecount;

					// Bugs
					if (DEBUG) {
						for (int i = 0; i < bugs; i++) {
							Console << U"1st";
							Console << U"BUG[{}], X:{} Y:{}, Dir{}"_fmt(i, cbug[i].x, cbug[i].y, cbug[i].direction);
						}
					}
					for (int i = 0; i < bugs; i++) {
						if (cbug[i].live) {
							cbug[i].odd_doing(drawmap, cx, cy);
							if ((Abs(cx - cbug[i].x) <= 1)
									&& (Abs(cy - cbug[i].y) <= 1)) { //擦り抜ける場合があるので、座標も確認
								playerstate = DEAD;
								cbug[i].erase(drawmap);
								eat.bugd = cbug[i].direction;

								if ((cbug[i].px == cx) && (cbug[i].py == cy)) {
									eat.bugx = (cbug[i].px + cbug[i].v.dx * 2) & 254;
									eat.bugy = (cbug[i].py + cbug[i].v.dy * 2) & 254;
									switch (eat.bugd) {
									case DIRUP:
										eat.bugd = DIRDOWN;
										break;
									case DIRDOWN:
										eat.bugd = DIRUP;
										break;
									case DIRRIGHT:
										eat.bugd = DIRLEFT;
										break;
									case DIRLEFT:
										eat.bugd = DIRRIGHT;
										break;
									}
								}
								else {
									eat.bugx = cbug[i].px;
									eat.bugy = cbug[i].py;
								}
								eat.manx = cx;
								eat.many = cy;
								if (DEBUG4) {
									Console << U"Odd Location Dead";
									Console << U"cx:{},cy:{}"_fmt(cx, cy);
									Console << U" bugx:{}, bugy:{}"_fmt(cbug[i].x, cbug[i].y);
									Console << U" buPx:{}, buPy:{}"_fmt(cbug[i].px, cbug[i].py);
									Console << U" buDx:{}, buDy:{}"_fmt(cbug[i].v.dx, cbug[i].v.dy);

								}
							}
						}
					}

					// Moving man hammer, stopper
					if (nextAct != DIRSTAY) {
						if (((cx % 2) == 0) && ((cy % 2) == 0)) {
							if (canMove(cx, cy, nextAct)) {
								moveMan();
							}
						}
					}
				}
			}
			else if (accumulator <= spawnTime * 2) {
				// 偶数描画で行う処理
				if (!frame1st) {
					frame1st = true;
					// Stopper
					for (int i = 0; i < 8; i++) {
						if (cstopper[i].live) {
							cstopper[i].doing(drawmap);
						}
					}
					// Bugs
					if (DEBUG) {
						for (int i = 0; i < bugs; i++) {
							Console << U"2nd";
							Console << U"BUG[" << Format(i) << U"], X:" << Format(cbug[i].x) <<
								U", Y:" << Format(cbug[i].y) << U", Dir:" << Format(cbug[i].direction);
						}
					}
					for (int i = 0; i < bugs; i++) {
						if (cbug[i].live) {
							cbug[i].even_doing(drawmap, cx, cy);
						}
					}
					//Print << U"Do2:Frames:" << framecount;
					if (playerstate != DEAD) {
						// live
						if (nextAct != DIRSTAY) {
							if ((((cx % 2) == 1) || (cy % 2) == 1)
							&& (canMove(cx, cy, nextAct)))
								moveMan();
						}
						if (hammerflag) {
							hammerMan();
							hammerflag = false;
						}
						if (stopperflag) {
							cstopperMan();
							stopperflag = false;
						}
					}
				}
			}
			else {

				accumulator -= spawnTime * 2;
				keyinfixed = false;
			}
		}
	};
	void cstopperMan() // put stopper;
	{
		for (int i = 0; i < 8; i++) {
			if (cstopper[i].live == false) {
				cstopper[i].put(drawmap, cx, cy, lastDir);
				break;
			}
		}
	}
	void hammerMan() // Hammer!!
	{
		// ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
		//  書いていない！！！！
		// ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
	}
	void moveMan()
	{
		Vector v;
		v.next(nextAct);

		// Print << U"CX:" << cx << U" CY:" << cy << U" DIR:" << nextAct;
		// Print << U"v.x:" << v.x << U" v.y:" << v.y;
		// Print << U"v.dx:" << v.dx << U" v.dy:" << v.dy;

			//LEFT:   3 -> (3-1) & 254 = 2, 4 -> (4-1) &254 = 2
			//RIGHT:  5 -> 5+2 &254 =6,     4 -> 4+2 &254 = 6
		if ((drawmap.map[(cx + v.x) & 254][(cy + v.y) & 254] & CDMASK) != WALL) {
			cx = cx + v.dx;
			cy = cy + v.dy;
		}

		if (cx < 0) {
			cx = 0;
		}
		if (cx > CWIDTH * 2 - 2) {
			cx = CWIDTH * 2 - 2;
		}
		if (cy < 0) {
			cy = 0;
		}
		if (cy > CHEIGHT * 2 - 2) {
			cy = CHEIGHT * 2 - 2;
		}
	}
	boolean canMove(int x, int y, int direction)
	{
		int dx = x, dy = y;
		if (direction == DIRSTAY) return true;

		Vector v;
		v.next(direction);
		dy = (dy + v.y) & 254;
		dx = (dx + v.x) & 254;

		if (((drawmap.map[dx][dy] & CDMASK) == WALL) || ((drawmap.map[dx][dy] & CDMASK) == STOPPER)) {
			return false;
		}
		return true;
	}

	void draw() const override
	{
		//ClearPrint();
		//Print << U"CX:" << cx << U" CY:" << cy;
		// Wall, others
		boolean DebugDraw = false;
		if (DebugDraw) {
			//
			for (int i = 0; i < drawmap.width; i++) {
				for (int j = 0; j < drawmap.height; j++) {
					int yy = j * 8, xx = i * 8;
					switch (drawmap.map[i][j]) {
					case PATH:
						Rect{ xx,yy,8 }.draw(Palette::Black);
						Line{ xx,yy,xx + 5, yy }.draw(1, Palette::White);
						Line{ xx,yy,xx, yy + 5 }.draw(1, Palette::White);
						break;
					case WALL:
						Rect{ xx,yy, 8 }.draw(Palette::Green);
						break;
					case STOPPER:
						Rect{ xx,yy, 8 }.draw(Palette::Green);
						break;
					case BUG + DIRSTAY:
						if ((i % 2 == 0) && (j % 2 == 0)) {
							Rect{ xx,yy,8 }.draw(Palette::Red);
						}
						else {
							Rect{ xx,yy,8 }.draw(Palette::Yellow);
						}
						break;
					case BUG + DIRUP: {
						LineString l{
							Vec2{xx,yy + 8},Vec2{xx + 4,yy},{xx + 8,yy + 8}
						};
						//Triangle t{
						//	static_cast<double>(xx),
						//	static_cast<double>(yy) + 8,
						//	static_cast<double>(xx)+ 4,
						//	static_cast<double>(yy),
						//	static_cast<double>(xx)+ 8,
						//	static_cast<double>(yy)+ 8
						//};
						if ((i % 2 == 0) && (j % 2 == 0)) {
							l.draw(2, Palette::Yellow);
						}
						else {
							l.draw(2, Palette::Red);
						}
						break;
					}
					case BUG + DIRDOWN: {
						LineString l{
							Vec2{xx,yy},Vec2{xx + 4,yy + 8},{xx + 8,yy}
						};
						if ((i % 2 == 0) && (j % 2 == 0)) {
							l.draw(2, Palette::Yellow);
						}
						else {
							l.draw(2, Palette::Red);
						}
						break;
					}
					case BUG + DIRRIGHT: {
						LineString l{
							Vec2{xx,yy},Vec2{xx + 8,yy + 4},{xx,yy + 8}
						};
						if ((i % 2 == 0) && (j % 2 == 0)) {
							l.draw(2, Palette::Yellow);
						}
						else {
							l.draw(2, Palette::Red);
						}
						break;
					}
					case BUG + DIRLEFT: {
						LineString l{
							Vec2{xx + 8,yy},Vec2{xx,yy + 4},{xx + 8,yy + 8}
						};
						if ((i % 2 == 0) && (j % 2 == 0)) {
							l.draw(2, Palette::Yellow);
						}
						else {
							l.draw(2, Palette::Red);
						}
						break;
					}
					}
				}
			}
			Rect{ cx * 8,cy * 8,16 }.draw(Palette::White);
		}

		else {
			for (int i = 0; i < 11 * 2; i++) {
				for (int j = 0; j < 7 * 2; j++) {
					//xx,yy はmap の配列インデックス
					int xx = cx - 11 + i;
					int yy = cy - 7 + j;
					//配列の範囲外であれば、飛ばす。
					if ((xx < 0) || (xx >= drawmap.width) || (yy < 0) || (yy >= drawmap.height)) {
						continue;
					}
					//if (yy == drawmap.height) Print << U"HEIGHT:" << drawmap.height;
					int dx = i * SPSIZE / 2 - SPSIZE / 2;
					int dy = j * SPSIZE / 2 + SPSIZE / 2;
					int c = drawmap.map[xx][yy];
					switch (c & CDMASK) {
					case WALL: // Wall
						if (((xx % 2) == 0) && ((yy % 2) == 0)) wall.draw(dx, dy);
						break;
					case STOPPER: // Wall
						stopper.draw(dx, dy);
						break;
					case BUG + DIRSTAY:
					case BUG + DIRUP:
					case BUG + DIRDOWN:
					case BUG + DIRRIGHT:
					case BUG + DIRLEFT:
					{
						int d = c & 3;
						if (d == 0) d = 4;
						d = d - 1;
						if (DEBUG3) Console << U"cbug:{} Bug:{},{},{},F{}"_fmt(cbug[0].direction, c, c & 3, d, frame1st);
						//bug[d][frame1st ? 0 : 1].draw(dx, dy);
						int t = (framecount / 8) & 1;
						if (playerstate == DEAD) break;
						bug[d][(t == 0) ? 0 : 1].draw(dx, dy);
						break;
					}
					case PATH: // Path
						// PATH(通路)は、偶数(左上)の時しか描画しない。
						if ((c == PATH) && ((xx % 2) == 0) && ((yy % 2) == 0)) path.draw(dx, dy);
						//if (((xx % 2) == 1) && ((yy % 2) == 0)) path.draw(dx, dy);
						break;
					case EATMAN: {

						int d = c & 3;
						if (d == 0) d = 4;
						d = d - 1;
						eaten[d][1].draw(dx, dy);
						break;
					}
					case EATBUG: {
						int d = c & 3;
						if (d == 0) d = 4;
						d = d - 1;
						eaten[d][0].draw(dx, dy);
						break;
					}
					default: {
						break;

					}

					}
#ifdef _DEBUG4_
					if ((xx % 2 == 0) && (yy % 2 == 0)) {
						Debugfont(U"{},{}"_fmt(xx, yy)).draw(dx, dy);
					}
#endif
				}
			}
			// Draw men.
			int t;
			if (frame1st) t = 0;
			else t = 1;
			// Print << U"half:" << t;
			if (playerstate != DEAD)man[nextAct][t].draw(5 * SPSIZE, 4 * SPSIZE);
		}
		// Rightside info
		int ly = 24 * 3, lx = 58 * 12;
		FontAsset(U"PC8001")(U"SCORE").draw(lx, ly);
		FontAsset(U"PC8001")(U"{:0>6}"_fmt(getData().score)).draw(lx, ly + 24 * 2);
		FontAsset(U"PC8001")(U"TIME").draw(lx, ly + 24 * 6);
		FontAsset(U"PC8001")(U"{:0>2}"_fmt(getData().time)).draw(lx, ly + 24 * 8);
		FontAsset(U"PC8001")(U"ALIEN").draw(lx, ly + 24 * 10);
		FontAsset(U"PC8001")(U"{}"_fmt(getData().aliens)).draw(lx + 12 * 3, ly + 24 * 12);
	}
};

void Main()
{
	FontAsset::Register(U"PC8001", 22, U"font/DotGothic16-Regular.ttf");
	FontAsset(U"PC8001").preload(U"　・！？．。（）［］｛｝＊／゛゜←→↑↓＜＝＞■ー０１２３４５６７８９ＡAｂＢＣCＥEＦＧＨＩIＬLMＭｎＮNＯOＰＲRＳSＴTＵＶＸｙＹアイウオカキクケコサシスセソタチッツテトナニネノハフヘホマムメモヤュラリルレロワヲン");
	ClearPrint();

	//Scene::SetResizeMode(ResizeMode::Keep);
	//Window::SetStyle(WindowStyle::Sizable);
	// ウィンドウの閉じるボタンを押すか、System::Exit() を呼ぶのを終了操作に設定,
	// エスケープキーを押しても終了しないようになる
	//System::SetTerminationTriggers(UserAction::CloseButtonClicked);

	App manager;

	manager.add<Title>(U"Title");
	manager.add<Instruction>(U"Instruction");
	manager.add<InitGame>(U"InitGame");   // ゲーム開始
	manager.add<InitScreen>(U"InitScreen"); // 面の初期化
	manager.add<StartGame>(U"StartGame");  // 中間報告の表示
	manager.add<Game>(U"Game");
	manager.add<GameOver>(U"GameOver");

	//manager.add<DebugDraw>(U"DebugDraw");
	//manager.init(U"DebugDraw");
//	manager.init(U"GameOver");


	while (System::Update())
	{
		if (not manager.update()) {
			break;
		}
	}
};
