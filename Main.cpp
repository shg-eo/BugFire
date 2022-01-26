/*======================================================================
Project Name : BUG FIRE!
File Name : Main.cpp
Creation Date : Oct/15/2021

Copyright © 2021,2022 shg-eo as SAKUMA, Shigeo. All rights reserved.
======================================================================*/
#include <Windows.h>
# include <Siv3D.hpp> // OpenSiv3D v0.6.2

// global variable.
// configuration, high score, global status.
struct Global
{
	int players = 3;	//conf
	int bugs = 8;	//conf
	int highscore = 0;	// high-score
	boolean ingame = false;	// global status;
	Stopwatch stopwatch;	//
	boolean enableSound = false;	//conf
	boolean NODEAD = false;	//conf
} global;

// 迷路のサイズ WxH
#define CWIDTH  33
#define CHEIGHT 33
// キャラクターの縦横サイズ(正方形)
#define SPSIZE  60

// stopper, bug sleeping time
constexpr int sleeptime = 350;
constexpr int stoppertime = 200;
// 何フレーム書いたら、timeを1減らすのか？
// 1フレームの目安は、 0.033秒。1秒ならば 30
constexpr int timedown = 512;

// enable/disable debug console
constexpr auto DEBUG = false;
constexpr auto DEBUG2 = false;
constexpr auto DEBUG3 = false;
constexpr auto DEBUG4 = false;

//#define _DEBUG_
//#define _DEBUG2_
//#define _DEBUG3_
//#define _DEBUG4_
//#define _NODEAD_

// mapping character num, mask bit.
constexpr auto PATH = 0;
/// @brief 方向抽出用 bit
constexpr auto DIRMASK = 7;   //  方向抽出用
/// @brief 3/4には、CMASKのみを書く。＊PATHには付けない＊
//                         10FEDCBA9876543210
constexpr auto CMASK   = 0b000000000000010000; // 左上以外でPATH以外にはこのbitを付ける。not used.
constexpr auto CDMASK  =~0b000000000000011111; // char抽出用のマスク not(31)
constexpr auto WALL    = 0b000000000000100000; // 32;
constexpr auto STOPPER = 0b000000000001000000; // 64;
constexpr auto MAN     = 0b000000000010000000; // 128;
constexpr auto BUG     = 0b000000000100000000; // 256;
constexpr auto EATMAN  = 0b000000001000000000; // 512;
constexpr auto EATBUG  = 0b000000010000000000; // 1024;
constexpr auto HUMMAN  = 0b000000100000000000; // 2048;
constexpr auto HUMBUG  = 0b000001000000000000; // 4096;
constexpr auto SLPBUG  = 0b000010000000000000; // 8192;
constexpr auto NHMMAN  = 0b000100000000000000; // 16384;
constexpr auto NHUMER  = 0b001000000000000000; // 32768;

// 0: no (path)
// 1: Wall
// 32: Wall2 (見えない壁)
// 2: Stopper
// 4,5,6,7,8: Man (Stop, U, D, R, L)
// 9,10,11,12: Bug (U,D,R,L)
// 13,14,15,16: Man with Hammer
// 17,18,19,20: Hammer
/*




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

	Maze()
	{
		for (int i = 0; i < width; i++)
		{
			for (int j = 0; j < height; j++)
			{
				maze[i][j] = 0;
			}
		}
	}

	// 迷路の作成
	//
	int Generate()
	{
		int level = 0;
		// 外周
		for (int i = 0; i < width; i++)
			for (int j = 0; j < height; j++)
				if ((i == 0) || (j == 0) || (i == (width - 1)) || (j == (height - 1)))
				{
					maze[i][j] = WALL;
				}
				else
				{
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
						level += 1;
						break;
					}

					// 1/2 で前と同じ方向
					if ((rnd() % 2) == 0)
					{
						level += 1;
						// 1行目のみ上に倒せる
						if (y == 2)
							direction = rnd() % 4;
						else
							direction = rnd() % 3;
					}
					else
					{
						// s3d::Console.WriteLine("SameDirection");
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
		// 	s3d::Console << s;
		// }

		// level は 150 ぐらい。大きいほど単純化するから、
		// 100 - lebel /3
		return (100 - (level / 3));
	}

};

// ゲーム中 の共有データ
struct GameData
{
	int bugs;	// number of bugs
	int score;	// score;
	int men;	// number of men
	int screen; // 面数
	int time;
	Maze maze;
	int mazelevel;
	double spawnTime; // 1/Framerate;
	int eaten;
};
using App = SceneManager<String, GameData>;


class GameSound {
	const Audio walk{      Audio::Stream, Resource(U"Sounds/Walk.mp3"),      Loop::No };
	const Audio eaten{     Audio::Stream, Resource(U"Sounds/Eaten.mp3"),     Loop::No };
	const Audio exit{      Audio::Stream, Resource(U"Sounds/Exit.mp3"),      Loop::No };
	const Audio hammerBug{ Audio::Stream, Resource(U"Sounds/HammerBug.mp3"), Loop::No };
	const Audio hammerNo{  Audio::Stream, Resource(U"Sounds/HammerNo.mp3"),  Loop::No };
public:
	boolean enableSound;


	GameSound()
	{
		enableSound = global.enableSound;
	}
	void playWalk()
	{
		if (enableSound)
		{
			walk.playOneShot();
			return;
		}
		;
	}
	void playHammerMan()
	{
		if (enableSound)
		{
			hammerBug.playOneShot();
			return;
		}
		;
	}
	void playHammerNo()
	{
		if (enableSound)
		{
			hammerNo.playOneShot();
			return;
		}
		;
	}
	void playEaten()
	{
		if (enableSound)
		{
			eaten.playOneShot();
			return;
		}
		;
	}
	void playExit()
	{
		if (enableSound)
		{
			exit.playOneShot();
			return;
		}
		;
	}
};

//
/// opening demo..
class Title : public App::Scene
{
private:
	int waitframecount;
	enum showorder : int
	{
		normal = 0,
		typing = 1,	// 1文字ずつ表示
		waittyping = 3,
	};

	typedef struct {
		showorder o;
		s3d::String m;
	} demos;

	demos demomessage[5] = {
		{normal, U" Ｏｋ"},
		{typing, U" ｍｏｎ"},
		{typing, U" ＊Ｌ"},
		{waittyping, U" ＊ＧＥ３ＣＣ"},
		{typing, U"  "},
	};

	template <typename T, std::size_t N>
	inline std::size_t sizeOfArray(const T(&)[N])
	{
		return N;
	}

	double spawntyping= 0.2; // frames;
	double spawnwaittyping = 1;
	double spawnnormal = 0.01;
	int x = 0, l = 0, meslines;
	double spawntime, accumlator;
	int maxlen;

public:

	// コンストラクタ（必ず実装）
	Title(const InitData& init)
		: IScene{ init }
	{
		meslines = (int)sizeOfArray(demomessage);
		l = 0;
		spawntime = spawntyping;
		accumlator = 0;
		maxlen = 0;
		for (int i = 0; i < meslines; i++) {
			maxlen += (int)demomessage[i].m.size();
		}
	}

	// 更新関数（オプション）
	void update() override
	{
		if (x != 0) {
			if (spawntime == spawnwaittyping)
			{
				spawntime = spawntyping;
			}
		}

		accumlator += Scene::DeltaTime();
		if (accumlator >= spawntime)
		{
			accumlator -= spawntime;
			// go ahead;
			x++;
			if (x > demomessage[l].m.size())
			{
				x = 0;
				l++;
				switch (demomessage[l].o)
				{
				case waittyping:
					spawntime = spawnwaittyping;
					break;
				case typing:
					spawntime = spawntyping;
					break;
				case normal:
					spawntime = spawnnormal;
					break;
				}
			}
		}

		if (l >= meslines) {
			l = meslines-1;
			// next schene;
			changeScene(U"Title2", 0s);
		}
	}

	// 描画関数（オプション）
	void draw() const override
	{
		for (int yy = 0; yy <= l; yy++)
		{
			String m = demomessage[yy].m;
			if (yy == l )
			{
				if (demomessage[yy].o != normal)
				{
					int t = m.size();
					for (int xx = 0; xx < (t - x - 1); xx++)
					{
						m.pop_back();
					}
				}
			}
			FontAsset(U"PC8001")(m).draw(20, yy * 24 + 24, Palette::White);
		}
	}
};


// Title Scene, Key bindings.
class Title2 : public App::Scene
{
private:
	int state = 0;
	int counts;
	double framerate = 0;
	Array <s3d::String> showingmessage = {
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
	int mState = 0;
	boolean showinstruction = false;

public:
	int SoundState = -1; // 1:EnableSound, 0:Mute Sound
	Title2(const InitData& init)
		: IScene{ init }
	{
		SoundState = -1;
		counts = 0;
		framerate = 0;
	}

	~Title2()
	{
		// 計測したframerateを設定
		//getData().spawnTime = framerate * 2.05;
		getData().spawnTime = framerate * 6;
	}

	void draw() const override
	{
		int ly = 0, lx = 20;
		for (int i = 0; i < showingmessage.size(); i++)
		{
			s3d::String m = showingmessage[i];
			FontAsset(U"PC8001")(m).draw(lx, ly, Palette::Lightgreen);

			ly += 24;
		}
	}

	void update() override
	{
		// Title でframerateを計測して、ゲーム開始時(Titleのデコンストラクタ)に6倍で設定する。
		// 極端に遅い場合の対策だけど、極端に速い場合の対策はしていない
		// 60Hzほぼ専用
		//
		if (counts < 100)
		{
			counts += 1;
			double t = Scene::DeltaTime();
			framerate = (framerate * counts + t) / (counts + 1);
		}

		boolean yes, no;
		yes = KeyY.down();
		no = KeyN.down();
		switch (mState)
		{
		case 0:		// Asking Sound;
			if (yes || no)
			{
				mState = 1;
				showingmessage << U"";
				showingmessage << U"　　　　セツメイ　カ゛　イリマスカ（ｙ／ｎ）";

				if (yes)
				{
					global.enableSound = false;
				}
				if (no)
				{
					global.enableSound = true;
				}
				break;
			}
			break;

		case 1:  // waiting relase key.
			if ((KeyY.pressed() == false)
				&& (KeyN.pressed() == false))
			{
				mState = 2;
			}
			break;

		case 2:
			if (yes || no) {
				mState = 3;

				if (yes)
				{
					showinstruction = true;
				}
				if (no)
				{
					showinstruction = false;
				}
			}
			break;

		case 3:
			if ((KeyY.pressed() == false)
				&& (KeyN.pressed() == false))
			{
				mState = 4;
			}
			break;

		case 4:
			if (showinstruction) {
				changeScene(U"Instruction", 0s);
			}
			else
			{
				changeScene(U"InitGame", 0s);
			}
			break;

		default:
			break;
		}
	}

};

class Instruction :public App::Scene
{
public:
	const s3d::String message[19] =
	{
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
		boolean yes, no;
		yes = KeyY.up();
		no = KeyN.up();

		if (yes)
		{
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
		for (s3d::String m : message)
		{
			//	font(U"SCORE").draw(300, 300);
			FontAsset(U"PC8001")(m).draw(lx, ly, Palette::Lightgreen);
			ly += 24;
		}
	}

};

class Mstring
{
public:
#include <format>
	/// @brief S 数値から、全角文字にする。マイナスには未対応
	/// @param m: 数値、digit:けたすう
	/// @return
	static String itoz(int m, int digit = 0, String sup = U"０")
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
			if (t == 0x002e) s += U"．";
		}
		if (digit > 0)
		{
			for (int i = 0; i < digit - a.length(); i++)
			{
				s = sup + s;
			}
		}
		return s;
	}

	// static String zupper(String s)
	// {
	// 	const String ZAU = U"ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺ";
	// 	const String HAL = U"abcdefghijklmnopqrstuvwxyz";
	// 	const String ZD = U"０１２３４５６７８９．－＋";
	// 	const String HD = U"0123456789.-+";
	// 	const String ZK = U"！＠＃＄％＾＆＊（）＿＋｜－＝＼［］｛｝；’＼：”，．／＜＞？";
	// 	const String HK = U"!@#$%^&*()_+|-=\\[]{};':\",./<>?";


	// }
};

//GameOver
class GameOver :public App::Scene
{
public:
	String message[21] =
	{
		U"",
		U"　　＊＊＊＊＊　Ｇ　Ａ　Ｍ　Ｅ　　Ｏ　Ｖ　Ｅ　Ｒ　＊＊＊＊＊",
		U"",
		U"",
		U"",
		U"",
		U"",
		U"　　　アナタ　ノ　スコア　ハ　{}　テン　テ゛シタ。"_fmt(Mstring::itoz(getData().score,6)),
		U"",
		U"                                                                       ",
		U"",
		U"",
		U"　　　アナタ　ノ　タッシタ　メンスウ　ハ　　{}　　テ゛シタ。"_fmt(Mstring::itoz(getData().screen,2)),
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
		if (getData().score > global.highscore)
		{
			global.highscore = getData().score;
			message[9] = U"　　　＊（コレ　ハ　ハイスコア　テ゛ス。）＊";
		}
		else
		{
		}

	}

	~GameOver()
	{

	}

	void update() override
	{
		boolean space;
		space = KeySpace.up();

		if (space)
		{
			// changeScene(U"Inform");
			changeScene(U"InitGame");
		}
	}

	void draw() const override
	{
		int ly = 12, lx = 3;
		for (s3d::String m : message)
		{
			//	font(U"SCORE").draw(300, 300);
			FontAsset(U"PC8001")(m).draw(lx, ly, Palette::Lightgreen);
			ly += 24;
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
		getData().men = global.players;
		getData().screen = 0;
	};

	~InitGame()
	{
#ifdef _DEBUG_
		if (DEBUG) Print << global.stopwatch.ms() << U"msec";
#endif

	}
	void update() override
	{
#ifdef _DEBUG_
		if (DEBUG) Print << U"Scene: InitGame";
#endif
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
#ifdef _DEBUG_
		if (DEBUG) Print << U"Scene: InitScreen";
#endif
		//面数の更新
		getData().screen += 1;
		//BUG数の更新
		getData().bugs = global.bugs;
		getData().time = 99;
		//迷路の初期化
		getData().mazelevel = getData().maze.Generate();

		global.ingame = false;

	}

	~InitScreen()
	{
#ifdef _DEBUG_
		if (DEBUG) Print << global.stopwatch.ms() << U"msec";
#endif
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
#ifdef _DEBUG_
		if (DEBUG) Print << U"StartGame:";
#endif
		// 中間報告の表示
		message[0] = U"　　　　　　　　　　　 チュウカンホウコク";
		message[1] = U"　　　　　タ゛イ　{}　メン"_fmt(Mstring::itoz(getData().screen, 2));
		message[2] = U"　　　　　メイロ　ノ　レヘ゛ル　　{}"_fmt(Mstring::itoz(getData().mazelevel, 2));
		message[3] = U"　　　　　モチシ゛カン　{}"_fmt(Mstring::itoz(getData().time, 2));
		message[4] = U"　　　　　フ゜レイヤー　ニンス゛ウ　{}"_fmt(Mstring::itoz(getData().men));
		message[5] = U"　　　　　ＳＣＯＲＥ　　{}"_fmt(Mstring::itoz(getData().score, 6));
		message[6] = U"　　　　　ＨＩＧＨ　ＳＣＯＲＥ　{}"_fmt(Mstring::itoz(global.highscore, 6));
		message[7] = U"　　　　　Ｆ　Ｉ　Ｇ　Ｈ　Ｔ　！！！！！！！";
	}

	~StartGame()
	{
#ifdef _DEBUG_
		if (DEBUG) Print << global.stopwatch.ms() << U"msec";
#endif
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
		for (int i = 0; i < 8; i++)
		{
			FontAsset(U"PC8001")(message[i]).draw(x, y, Palette::White);
			y += 48;
		}
	}
};

class Map4draw
{
public:
	int map[CWIDTH][CHEIGHT + 1];
	int width;
	int height;
	void init(Maze maze)
	{
		height = CHEIGHT + 1;
		width = CWIDTH;
		for (int xx = 0; xx < CWIDTH; xx++)
		{
			for (int yy = 0; yy < CHEIGHT; yy++)
			{
				map[xx][yy] = maze.maze[xx][yy]; // 左上
			}
		}
		//最下行
		for (int i = 0; i < width; i++)
		{
			map[i][height - 1] = WALL;
		}
		height = CHEIGHT + 1;
		width = CWIDTH;
	}
};


class Game :public App::Scene
{

public:
#ifdef _DEBUG4_
	const Font Debugfont{ FontMethod::SDF, 12, Typeface::Bold };
	const Vec2 shadowOffset{ 4,4 };
	const ColorF shadowColor{ 0.0,0.5 };
#endif
	double time;

	//int map[CWIDTH * 2][CHEIGHT * 2]; // mapping
	Map4draw drawmap; // classに変更

	GameSound Sound;

	/// @brief PlayerState
	enum Playerstate
	{
		NORMAL, DEAD, HUMMER, NOHUMMER, EXIT, PAUSE,
	};
	Playerstate playerstate,prevstate;
	boolean pause = false;

	class Vector
	{
	public:
		int x, y, dx, dy;
		Vector()
		{
			x = y = dx = dy = 0;
		}

		Vector next(int dir, int xx = 0, int yy = 0)
		{
			Vector ret;
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
				y = 1; dy = 1;
				break;
			case DIRRIGHT:
				x = 1; dx = 1;
				y = 0; dy = 0;
				break;
			case DIRLEFT:
				x = -1; dx = -1;
				y = 0; dy = 0;
				break;
			default:
				break;
			}
			ret.x = xx + dx;
			ret.y = yy + dy;

			return ret;
		}

		Vector prev(int dir, int xx = 0, int yy = 0)
		{
			Vector ret;
			switch (dir)
			{
			case DIRSTAY:
				break;
			case DIRUP:
				y = 1; dy = 1;
				break;
			case DIRDOWN:
				y = -1; dy = -1;
				break;
			case DIRRIGHT:
				x = -1; dx = -1;
				break;
			case DIRLEFT:
				x = 1; dx = 1;
				break;
			default:
				break;
			}
			ret.x = xx + dx;
			ret.y = yy + dy;

			return ret;
		}
	};

	class Char
	{
	public:
		int x, y;
		int character; //bug, stopperとか
		boolean live; // 存在するかどうか(bugはデフォルトでt, stopperはｆ)

		Char()
		{ // 引数なしのコンストラクタを必ず作成すること。
			x = -1; // -1 : not on screen;
			y = -1;
			live = true;
		}
		Char(boolean l)
		{
			x = -1; // -1 : not on screen;
			y = -1;
			live = l;
		}
		~Char()
		{

		}

		void draw(Map4draw& drawmap, int dir = 0) // mapに描画
		{
			if (drawmap.map[x][y] != 0)
			{
#ifdef _DEBUG_
				if (DEBUG) s3d::Console << U"Draw not 0" << U" X" << x << U" Y" << y
					<< U", ch:" << drawmap.map[x][y];
#endif
			}
			// 実際の画面への描写時は左上の情報だけで描画させるため、
			// 左上以外は、bitを立てる。
			drawmap.map[x][y] = character + dir;
		}
		void erase(Map4draw& drawmap) // mapに描画
		{
			drawmap.map[x][y] = 0;
		}
	};


	// 各キャラクターのクラス
	// Charクラスから継承

	// d3dのシーンで共有する必要の無い共有する変数
	//Maze maze;	// maze
	int cx, cy;		// man position
	int nextDir;	// man next direction
	int lastDir;	// man last direction (for stopper);
	int keyDir;	    // key in dir;
	Vector cv;		// man vector;
	int hx, hy;		// Hammer location.

	struct Eat
	{
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
	int framecount = 0;
	const int frameflow = 100;

#ifdef _DEBUG4_
	int framestate;
#endif

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

			int lx = (cx + v.x);
			int ly = (cy + v.y);

			if (drawmap.map[lx][ly] == PATH)
			{
				x = lx;
				y = ly;
				draw(drawmap);
				live = true;
				lifetime = stoppertime;
			}

		}
		void remove(Map4draw& drawmap)
		{
			if ((drawmap.map[x][y] & CDMASK) == STOPPER)
			{
				erase(drawmap);
			}
			live = false;
		}
		void doing(Map4draw& drawmap)
		{
			lifetime -= 1;
			if (lifetime <= 0)
			{
				remove(drawmap);
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
			live = true;
			character = BUG;
			// default Location;
			x = CWIDTH / 2 + 1;
			y = 1;
			px = x;
			py = y;
			direction = 0;
			sleep = 0;
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

		// BUG を動かす。
		void doing(Map4draw& drawmap, int cx, int cy, Playerstate playstate)
		{
#ifdef _DEBUG4_
			if (playstate != NORMAL) Console << U"MOVING at not NORMAL {}"_fmt(playstate);
#endif
			// 方向を決める。
			std::random_device rnd;
			int d;
			String s = U"";

			// exit in sleeping;
			if (sleep > 0)
			{
				erase(drawmap);
				draw(drawmap, direction);
				//direction = 0;
				sleep--;
				if (sleep != 0) {
					return;
				}
				character = BUG;
				//return;
			}

			v.dx = v.dy = 0;

			// まずは自分を消す。
			erase(drawmap);

			// 動ける方向を格納
			Array <int> canmove;

			for (int i = 1; i <= 4; i++)
			{
				v.next(i);

				int x1 = x + v.dx;
				int y1 = y + v.dy;

				if ( // map の範囲内 でないと、アクセス不可のため、事前に判定
					(x1 > 0) && (x1 < drawmap.width)
				 && (y1 > 0) && (y1 < drawmap.height)
					)
				{
					if (
						((drawmap.map[x1][y1] & CDMASK) != WALL)
						&&
						((drawmap.map[x1][y1] & CDMASK) != BUG)
						&&
						((drawmap.map[x1][y1] & CDMASK) != SLPBUG)
						)
					{
						canmove << i;
#ifdef _DEBUG_
						if (DEBUG)s3d::Console << U"CanMoveFound" << Format(i);
#endif
					}
				}
			}

			// 動ける方向が無い場合は、d (方向)を0にして動かさない。
			if (canmove.size() == 0)
			{
				d = 0;
			}
			else
			{
				// 前と同じ方向に動けるのか？
				boolean canmoveprevious = false;
				for (int i = 0; i < canmove.size(); i++)
				{
					if (canmove[i] == direction) canmoveprevious = true;
				}

				//動けた場合、1/n の確率で同じ方向
				if ((canmoveprevious) && ((rnd() % 5) >= 1))
				{
					d = direction;
				}
				// else { // 1/n の確率で、反対方向
				// }
				else
				{ //そうでなければ、ランダム
					d = canmove[rnd() % canmove.size()];
				}
			}

			direction = d;
			v.next(direction);

#ifdef _DEBUG_
			if (DEBUG)
			{
				s3d::Console << U"Selected dir:" << d;
				s3d::Console << U"X:{} -> {}, dx:"_fmt(x, x + v.dx, v.dx);
				s3d::Console << U"Y:{} -> {}, dy:"_fmt(y, y + v.dy, v.dy);
			}
#endif

			//動かす前の処理
			if (direction != 0)
			{
				int vx = x + v.dx;
				int vy = y + v.dy;

				if (vx <= 1) vx = 1;
				if (vy <= 1) vy = 1;
				if (vx >= drawmap.width - 1)  vx = drawmap.width - 1;
				if (vy >= drawmap.height - 1) vy = drawmap.height - 1;

				if ((drawmap.map[vx][vy] & CDMASK) == STOPPER) {
					sleep = sleeptime;
					character = SLPBUG;
				}
				else
				{
					px = x;
					py = y;
					x = vx;
					y = vy;
				}
			}
			draw(drawmap, d);
		}
	};

	// Characters...
	CStopper cstopper[8];
	CBug *cbug;

	// Texture Define..
	const Texture bug[4][2] =
	{  // Bug Direction UDRL x2 (Animation)
		{ Texture(Resource(U"img/BugU1.png")), Texture(Resource(U"img/BugU2.png"))},
		{ Texture(Resource(U"img/BugD1.png")), Texture(Resource(U"img/BugD2.png"))},
		{ Texture(Resource(U"img/BugR1.png")), Texture(Resource(U"img/BugR2.png"))},
		{ Texture(Resource(U"img/BugL1.png")), Texture(Resource(U"img/BugL2.png"))},
	};

	// Texture Define..
	const Texture eaten[4][2] =
	{  // Bug , man
		{ Texture(Resource(U"img/EatBU.png")), Texture(Resource(U"img/EatMU.png"))},
		{ Texture(Resource(U"img/EatBD.png")), Texture(Resource(U"img/EatMD.png"))},
		{ Texture(Resource(U"img/EatBR.png")), Texture(Resource(U"img/EatMR.png"))},
		{ Texture(Resource(U"img/EatBL.png")), Texture(Resource(U"img/EatML.png"))},
	};

	const Texture man[5][2] =
	{  // Man Direction UDRLS x2(Animation)
		{ Texture(Resource(U"img/ManSt.png")), Texture(Resource(U"img/ManSt.png"))},
		{ Texture(Resource(U"img/ManU1.png")), Texture(Resource(U"img/ManU2.png"))},
		{ Texture(Resource(U"img/ManD1.png")), Texture(Resource(U"img/ManD2.png"))},
		{ Texture(Resource(U"img/ManR1.png")), Texture(Resource(U"img/ManR2.png"))},
		{ Texture(Resource(U"img/ManL1.png")), Texture(Resource(U"img/ManL2.png"))},
	};

	const Texture hman[4][2] =
	{  // Man + Hammer Direction UDRL x2(Man + Hammer)
		{ Texture(Resource(U"img/HmanU.png")), Texture(Resource(U"img/HumU.png"))},
		{ Texture(Resource(U"img/HmanD.png")), Texture(Resource(U"img/HumD.png"))},
		{ Texture(Resource(U"img/HmanR.png")), Texture(Resource(U"img/HumR.png"))},
		{ Texture(Resource(U"img/HmanL.png")), Texture(Resource(U"img/HumL.png"))},
	};
	const Texture nhman[4][2] =
	{  // Man + Hammer Direction UDRL x2(Man + Hammer)
		{ Texture(Resource(U"img/NmanU.png")), Texture(Resource(U"img/NhumU.png"))},
		{ Texture(Resource(U"img/NmanD.png")), Texture(Resource(U"img/NhumD.png"))},
		{ Texture(Resource(U"img/NmanR.png")), Texture(Resource(U"img/NhumR.png"))},
		{ Texture(Resource(U"img/NmanL.png")), Texture(Resource(U"img/NhumL.png"))},
	};
	//Texture wall;
	const Texture wall    = Texture(Resource(U"img/Wall.png"));
	const Texture stopper = Texture(Resource(U"img/Stopper.png"));
	const Texture path    = Texture(Resource(U"img/PATH.png"));

	Game(const InitData& init)
		:IScene{ init }
	{
		cbug = new CBug[global.bugs];

		drawmap.init(getData().maze);
		// start location 32,64 (16,32)
		cx = int(CWIDTH / 2);
		cy = (CHEIGHT - 2) + 1;
		nextDir = 0;
		lastDir = 0;
		cv;

		eat.bugx = eat.bugy = eat.bugd = 0;
		eat.manx = eat.many = 0;

		accumulator = 0;

		frame1st = true;
		framecount = 0;

		keyinfixed = false;

		hammerflag = false;
		stopperflag = false;

		playerstate = NORMAL;
#ifdef _DEBUG4_
		Console << U"State:NORMAL";
#endif

		for (int i = 0; i < global.bugs; i++)
		{
			cstopper[i].init();
			// cbug[i].init(); // Reset bugs before Schese
		}

		if (!global.ingame)
		{
			for (int i = 0; i < global.bugs; i++)
			{
				cbug[i].init(); // Reset bugs before Schese
			}
		}

		// 死亡後などで、bugが8匹分初期化されるため、消した分を殺す。
		for (int i = 0; i < global.bugs - getData().bugs; i++)
		{
			cbug[i].live = false;
			cbug[i].x = -1;
			cbug[i].y = -1;
		}

		// すべてのバグを消したときの処理
		if (getData().bugs == 0)
		{
			drawmap.map[(drawmap.width - 1) / 2][0] = PATH; // Exit
			cbug[0].x = CWIDTH / 2 + 1;
			cbug[0].y = 1;
			cbug[0].py = CWIDTH / 2 + 1;
			cbug[0].px = 1;
			cbug[0].direction = DIRDOWN;
			cbug[0].sleep = 0;
			cbug[0].character = BUG;
			cbug[0].live = true;
		}


#ifdef _DEBUG_
		if (DEBUG)
		{
			Vector v;
			s3d::Console << U"VectorCheck:";
			for (int i = 0; i < 6; i++)
			{
				v.next(i);
				s3d::Console << U"Dir:" << Format(i) << U" dx:" << Format(v.dx) << U" dy:" << Format(v.dy);
			}
		}
#endif

#ifdef _DEBUG2_
		if (DEBUG2)s3d::Console << U"Start!";
#endif
		global.ingame = true;

		getData().eaten = 0;
		double d = getData().spawnTime;
		if (d > 0)
		{
			if (d > 0.03)
			{  // 60Hz (0.03333)より速いレートは設定しない。
				spawnTime = getData().spawnTime;
			}
		}
		//spawnTime = 0.16;
	}

public:
	double spawnTime = 0.03333; // Moving frame [s]
	//double framspersec = 5;
	double accumulator;

	void update() override
	{
		//debug showing map info in s3d::Console...
#ifdef _DEBUG2_
		if constexpr (DEBUG2) if (KeySpace.pressed())
		{
			String s;
			s3d::Console << U"cx:{} cy:{} Frame:{}"_fmt(cx, cy, frame1st);

			for (int yy = cy - 13; yy <= cy + 13; yy++)
			{
				s = U"";
				for (int xx = cx - 12; xx <= cx + 12; xx++)
				{
					if ((cx >= 0) && (cx < drawmap.width) && (cy >= 0) && (cy < drawmap.height))
					{
						if ((xx == cx) && (yy == cy))
						{
							s = s + U"*{:>2},"_fmt(drawmap.map[xx][yy]);
						}
						else
						{
							s = s + U"{:>3},"_fmt(drawmap.map[xx][yy]);
						}
					}
					else
					{
						s = s + U"xxx ";
					}
					if (Abs(xx) % 2 == 1)
					{
						s = s + U" | ";
					}
				}
				s3d::Console << s;
				if (Abs(yy) % 2 == 1)
				{
					s3d::Console << U"------";
				}
			}
			return;
		}
#endif

		if (KeyEscape.down()) {
			pause = !pause;
		}
		if (pause) {
			accumulator = 0;
			return;
		}
		accumulator += Scene::DeltaTime();
		switch (playerstate)
		{
		case Game::DEAD:
		case Game::HUMMER:
		case Game::NOHUMMER: {
			if (accumulator >= spawnTime)
			{
				accumulator -= spawnTime;
			}
			// Dead!!
			if (timer.isRunning())
			{
				if (
					(timer >= 1.0s) && (playerstate == DEAD)
					||
					(timer >= 1.2s) && (playerstate == HUMMER)
					||
					(timer >= 1.2s) && (playerstate == NOHUMMER)
					)
				{//stop erase
					timer.reset();

					if ((playerstate == DEAD)
						||
						(getData().score < 0))
					{
						if (getData().score < 0) {
							getData().score = 0;
						}
						int p = (getData().men -= 1);
						if (p <= 0)
						{
							changeScene(U"GameOver", 0s);
						}
						else
						{
							changeScene(U"StartGame", 0s);
						}
					}
					else
					{
						prevstate = playerstate;
						// 時間が来たので、消す。
						playerstate = NORMAL;
#ifdef _DEBUG4_
						Console << U"State:NORMAL";
#endif

#ifdef _DEBUG4_
						String s;
						for (int i = 0; i < bugs; i++) {
							s += U"I{}:{},{} "_fmt(i, cbug[i].x, cbug[i].y);
						}
						Console << s;
#endif
						// Hummer Flagも戻して、hummerを出せるようにする。
						//hammerflag = false;

						drawmap.map[cx][cy] = PATH;
						drawmap.map[hx][hy] = PATH;
						// 方向もクリアする。
						nextDir = 0;
						lastDir = 0;
					}
				}
				else
				{
					// nothing to do.
				}
			}
			else
			{// Timer Start and Draw death.
				timer.start();
				if (playerstate == DEAD)
				{
					Sound.playEaten();
					drawmap.map[eat.bugx][eat.bugy] = EATBUG | eat.bugd;
					drawmap.map[eat.manx][eat.many] = EATMAN | eat.bugd;
				}
				if (playerstate == HUMMER)
				{
					Sound.playHammerMan();
					// drawmap.map[eat.bugx][eat.bugy] = EATBUG | eat.bugd;
					// drawmap.map[eat.manx][eat.many] = EATMAN | eat.bugd;
				}
				if (playerstate == NOHUMMER)
				{
					Sound.playHammerNo();
					// drawmap.map[eat.bugx][eat.bugy] = EATBUG | eat.bugd;
					// drawmap.map[eat.manx][eat.many] = EATMAN | eat.bugd;
				}
			}
			break;
		}
						   break;
		case Game::EXIT:
			if (cy < 0)
			{
				cy = 0;
			}
			if (timer.isRunning())
			{
				if (timer >= 2s) {
					getData().score += getData().time;
					changeScene(U"InitScreen", 0.5s);
				}

			}
			else
			{
				Sound.playExit();
				timer.start();
			}
			break;
		case Game::NORMAL: {
#ifdef _DEBUG4_
			if (playerstate != NORMAL) {
				Console << U"NotNORMAL!!";
			}
#endif
			//keyinfixed = true;
			stopperflag = false;
			hammerflag = false;
			if (!keyinfixed)
			{
				boolean
					upKey = (KeyUp    | KeyNum8 | KeyW).pressed(),
					dnKey = (KeyDown  | KeyNum2 | KeyS).pressed(),
					rtKey = (KeyRight | KeyNum6 | KeyD).pressed(),
					lfKey = (KeyLeft  | KeyNum4 | KeyA).pressed(),
					hmKey = (KeyShift | KeyControl ).pressed(),
					stKey = (KeyX     | KeySpace).pressed();
				if (lfKey)
				{
					nextDir = DIRLEFT;
				}
				else if (rtKey)
				{
					nextDir = DIRRIGHT;
				}
				else if (dnKey)
				{
					nextDir = DIRDOWN;
				}
				else if (upKey)
				{
					nextDir = DIRUP;
				}

				if (nextDir != 0) keyDir = nextDir;

				// 方向入力が無く、keyinが確定していなければ、方向を0にリセットする。
				if (!upKey && !dnKey && !rtKey && !lfKey && !keyinfixed)
				{
					nextDir = 0;
				}
				else
				{
					if (!canMove(cx, cy, nextDir))
					{
						nextDir = 0;
					}
				}

				if (hmKey)
				{
					hammerflag = true;
					stopperflag = false;
				}
				if (stKey)
				{
					stopperflag = true;
					hammerflag = false;
				}
			}

			if (accumulator >= spawnTime)
			{
				accumulator -= spawnTime;

				// count TIMEl
				framecount = (framecount + 1) % frameflow;
				if (!framecount) getData().time--;

				if (getData().time < 0) {
					if (--getData().men <= 0) {
						changeScene(U"GameOver", 0s);
					}
					else
					{
						changeScene(U"StartGame", 0s);
					}
				}

#ifdef _DEBUG4_
				framestate = 1;
#endif
#ifndef _NODEAD_
				// Bugに食われているか？
				if (!global.NODEAD) {  // 死亡しないモード
					int bugno = -1;
					for (int i = 0; i < global.bugs; i++)
					{
						if (
							(cbug[i].live)
							&&
							(cbug[i].x == cx) && (cbug[i].y == cy)
							)
						{
							bugno = i;
							prevstate = playerstate;
							break;
						}
					}
					if (bugno != -1)
					{
						playerstate = DEAD;
#ifdef _DEBUG4_
						Console << U"State:DEAD";
#endif // _DEBUG4_

						eat.manx = cx;
						eat.many = cy;
						eat.bugx = cbug[bugno].px;
						eat.bugy = cbug[bugno].py;
						eat.bugd = cbug[bugno].direction;

						prevstate = playerstate;
						// break; ■■■■■■■■■■■breakでいいのか？？？
					}
				}
#endif // _NODEAD_


				// ハンマー処理
				if (hammerflag)
				{
					//hammerflag = false;
					if (nextDir == 0)
					{
						if (keyDir != 0)
						{
							hammerMan(keyDir);
						}
					}
					else
					{
						hammerMan(nextDir);
					}
					prevstate = playerstate;
					break;
				}
				//STOPPER処理
				if (stopperflag)
				{
					cstopperMan(keyDir);
					stopperflag = false;
				}

				//bug を 移動する
				for (int i = 0; i < global.bugs; i++)
				{
					if (cbug[i].live)
					{
						cbug[i].doing(drawmap, cx, cy, playerstate);
					}
				}

				// stopper 処理
				for (int i = 0; i < 8; i++) {
					if (cstopper[i].live)
					{
						cstopper[i].doing(drawmap);
					}
				}

				//自身を移動する。
				if (nextDir != 0) lastDir = nextDir;
				if (!canMove(cx, cy, nextDir))
				{
					nextDir = 0;
				}
				else
				{
					if (nextDir != DIRSTAY)
					{
						moveMan();
					}
				}
				//出口か？
				if (cy == 0) {
					playerstate = EXIT;
				}
			}
			prevstate = playerstate;
		}
		}
	};

	void cstopperMan(int dir) // put stopper;
	{
		for (int i = 0; i < 8; i++)
		{
			if (cstopper[i].live == false)
			{
				cstopper[i].put(drawmap, cx, cy, dir);
				break;
			}
		}
	}
	void hammerMan(int mandirection) // Hammer!!
	{
		Vector v;
		v.next(mandirection);
		hx = (cx + v.x);
		hy = (cy + v.y);
#ifdef _DEBUG4_
		//s3d::Console << U"Hummer! cx:{}, cy:{}"_fmt(cx + v.x, cy + v.y, lastDir);
#endif

#ifndef _DEBUG4_
		// ハンマーを振ると無条件で -10
		getData().score -= 10;
#endif

		if (
			((drawmap.map[hx][hy] & CDMASK) != WALL)
			&&
			((drawmap.map[hx][hy] & CDMASK) != STOPPER)
			)
		{
			// ハンマーを出せる

#ifdef _DEBUG4_
			//s3d::Console << U"Ham:OK {}"_fmt(drawmap.map[hx][hy]);
#endif
			nextDir = DIRSTOP;

			Vector vec;
			vec.next(mandirection);
#ifdef _DEBUG4_
				String s;
				for (int i = 0; i < bugs; i++) {
					s += U"I{}:{},{} "_fmt(i, cbug[i].x, cbug[i].y);
				}
				Console << s;
#endif
			if (
				((drawmap.map[hx + vec.dx][hy + vec.dy] & CDMASK) == BUG)
				||
				((drawmap.map[hx][hy] & CDMASK) == SLPBUG)
				||
				((drawmap.map[hx][hy] & CDMASK) == BUG)
				)
			{
				//バグがいる
#ifdef _DEBUG4_
				//s3d::Console << U"Hummer! cx:{}, cy:{}, l:{}"_fmt(cx, cy, lastDir);
#endif
				playerstate = HUMMER;
#ifdef _DEBUG4_
				Console << U"State:HUMMER";
#endif

				if (
					((drawmap.map[hx][hy] & CDMASK) == SLPBUG)
					||
					((drawmap.map[hx][hy] & CDMASK) == BUG)
					)
				{
					vec.dx = 0;
					vec.dy = 0;
				}
#ifdef _DEBUG4_
				s3d::Console << U"Hummer::BUG";
				if ((drawmap.map[hx + v.dx][hy + v.dy] & CDMASK) == BUG) {
					s3d::Console << U"Direct BUG hummer";
				}
#endif
				int bugnum = -1;

				//何番の bug かを見極める。
				for (int i = 0; i < global.bugs; i++) {
					if (
						((cbug[i].x == hx) && (cbug[i].y == hy))
						||
						((cbug[i].x == hx + vec.dx) && (cbug[i].y == hy + vec.dy))
						)
					{
						if (cbug[i].live)
						{
							bugnum = i;
							break;
						}
					}
				}

				if (bugnum == -1) {
					// 該当するbugがいない>？？？？？？
					Print << U"HALT!!  Hummer bug, but no bugs????";
					while (1);  // ■■■■■■■■■■■■■■■■■■ありえないか、確認すること！
				}
				else
				{
					//スコアの計算
					// 10 * (残りのbug数)、ただし、ハンマー1回で -10なので、
					// 1匹目 70, 60.. で、最後は 潰しても0点 (オリジナルのまま)
					getData().score += 10 * getData().bugs;
					int bs = getData().bugs -= 1;
					if (bs < 0)
					{
						bs = 0;
					}
					getData().bugs = bs;

					drawmap.map[hx + vec.dx][hy + vec.dy] = PATH;
					drawmap.map[hx][hy] = HUMBUG + mandirection;
					drawmap.map[cx][cy] = HUMMAN + mandirection;

					if (getData().bugs > 0) {

						// bugを無くして
						cbug[bugnum].live = false;
						cbug[bugnum].x = -1;	// 死んだbugに食われないようにするため、座標外にする。

					}
					else
					{
						// バグを全て潰した。
						drawmap.map[(drawmap.width - 1) / 2][0] = PATH; // Exit
						cbug[bugnum].x = CWIDTH / 2 + 1;
						cbug[bugnum].y = 1;
						cbug[bugnum].py = CWIDTH / 2 + 1;
						cbug[bugnum].px = 1;
						cbug[bugnum].direction = DIRDOWN;
						cbug[bugnum].sleep = 0;
						cbug[bugnum].character = BUG;
						getData().bugs = 0;
						accumulator = 0;
					}

					/*
					// バグを全て潰した。
					if (getData().bugs == 0) {
						// 出口を開けて、bugを1匹生き返えらせる。
						drawmap.map[(drawmap.width - 1) / 2][0] = PATH; // Exit
						// cbug[0].init();
						{
							// なぜか、上の行のコンストラクタがうまくいっていないため、
							// 直書きで初期化
							cbug[bugnum].x = CWIDTH / 2 + 1;
							cbug[bugnum].y = 1;
							cbug[bugnum].py = CWIDTH / 2 + 1;
							cbug[bugnum].px = 1;
							cbug[bugnum].direction = 0;
							cbug[bugnum].sleep = 0;
							cbug[bugnum].live = true;
						}
						getData().bugs = 1;
					}
					*/
				}
			}
			else
			{
				//バグがいない
				playerstate = NOHUMMER;
#ifdef _DEBUG4_
				Console << U"State:NoHummer";
#endif

				drawmap.map[hx][hy] = NHMMAN + mandirection;
				drawmap.map[cx][cy] = NHUMER + mandirection;

#ifdef _DEBUG4_
				//s3d::Console << U"Hummer::No BUG";
#endif
			}
		}
		else
		{
#ifdef _DEBUG4_
			// s3d::Console << U"Ham:NG {}"_fmt(drawmap.map[hx][hy]);
#endif
			return;
		}

	}
	void moveMan()
	{
		Vector v;
		v.next(nextDir);

		if (
			((drawmap.map[(cx + v.x)][(cy + v.y)] & CDMASK) != WALL)
			&&
			((drawmap.map[(cx + v.x)][(cy + v.y)] & CDMASK) != STOPPER)
			&&
			((drawmap.map[(cx + v.x)][(cy + v.y)] & CDMASK) != SLPBUG)
			)
		{
			cx = cx + v.dx;
			cy = cy + v.dy;
			Sound.playWalk();
		}

		if (cx < 0)
		{
			cx = 0;
		}
		if (cx > CWIDTH)
		{
			cx = CWIDTH - 1;
		}
		if (cy < 0)
		{
			cy = 0;
		}
		if (cy > CHEIGHT)
		{
			cy = CHEIGHT - 1;
		}
	}
	boolean canMove(int x, int y, int direction)
	{
		int dx = x, dy = y;
		if (direction == DIRSTAY) return true;

		Vector v;
		v.next(direction);
		dy = (dy + v.y);
		dx = (dx + v.x);

		if (
			((drawmap.map[dx][dy] & CDMASK) == WALL)
			||
			((drawmap.map[dx][dy] & CDMASK) == STOPPER)
			||
			((drawmap.map[dx][dy] & CDMASK) == SLPBUG)
			)
		{
			return false;
		}

		return true;
	}

	void draw() const override
	{
		if (pause) {
			FontAsset(U"PC8001")(U"ＰＡＵＳＥ").draw(300, 200, Palette::White);
			return;
		}

		int f = ((spawnTime / 2) <= accumulator) ? 1 : 0;
		int delta=0;
		int ddx = 0, ddy = 0;
		if (
			(playerstate == NORMAL)
			&&
			(prevstate != HUMMER)
			&&
			(prevstate != NOHUMMER)
			)
		{
			delta = int(((accumulator / spawnTime)) * SPSIZE * 11 / 12) - 1;
			//delta = f * SPSIZE / 2;
			if (nextDir == DIRUP)    ddy = -delta;
			if (nextDir == DIRDOWN)  ddy = delta;
			if (nextDir == DIRRIGHT) ddx = delta;
			if (nextDir == DIRLEFT)  ddx = -delta;
		}

		int limitx = 11, limity = 7;
		if (ddx > 0) limitx++;
		for (int i = -1; i < limitx; i++)
		{
			for (int j = -1; j < limity; j++)
			{
				//xx,yy はmap の配列インデックス
				int xx = cx - 11 / 2 + i;
				int yy = cy - 7 / 2 + j;
				//配列の範囲外であれば、飛ばす。
				if ((xx < 0) || (xx >= drawmap.width) || (yy < 0) || (yy >= drawmap.height))
				{
					continue;
				}
				int dx = i * SPSIZE - ddx;
				int dy = j * SPSIZE - ddy;
				int c = drawmap.map[xx][yy];
				switch (c & CDMASK)
				{
				case PATH: // Pathは描画しない。中間にかかっているbugを消してしまうため
					//path.draw(dx, dy);
					break;

				case WALL: { // Wall
					int drx = SPSIZE, dry = SPSIZE;
					if (i == limitx - 1)
					{
						if (ddx > 0) drx = ddx;
						else if (ddx < 0) drx = SPSIZE + ddx;
					}
					if (j == limity - 1)
					{
						if (ddy > 0) dry = ddy;
						else if (ddy < 0) dry = SPSIZE + ddy;
					}
					wall(0, 0, drx, dry).draw(dx, dy);
					break;
				}

				case STOPPER: // Wall
					stopper.draw(dx, dy);
					break;

				case SLPBUG:
				case BUG:
				{
					if (playerstate == DEAD) break;
					int d = c & 3;
					if (d == 0) d = 4;
					d = d - 1;
#ifdef _DEBUG4_
					//Console << U"xx,yy:{},{} dir:{}"_fmt(xx, yy, d);
#endif
#ifdef _DEBUG3_
					if (DEBUG3) s3d::Console << U"cbug:{} Bug:{},{},{},F{}"_fmt(cbug[0].direction, c, c & 3, d, f);
#endif
					if (
						((c & CDMASK) != SLPBUG)
						&&
						(playerstate == NORMAL)
						&&
						(prevstate != HUMMER)
						&&
						(prevstate != NOHUMMER)
						)
					{
						switch (d + 1) {
						case DIRUP:    dy += -delta + SPSIZE; break;
						case DIRDOWN:  dy -= -delta + SPSIZE; break;
						case DIRRIGHT: dx -= -delta + SPSIZE; break;
						case DIRLEFT:  dx += -delta + SPSIZE; break;
						default: break;
						}
					}
					bug[d][f].draw(dx, dy);

					break;
				}
				case EATMAN:
				{
					int d = c & 3;
					if (d == 0) d = 4;
					d = d - 1;
					eaten[d][1].draw(dx, dy);
					break;
				}
				case EATBUG:
				{
					int d = c & 3;
					if (d == 0) d = 4;
					d = d - 1;
					eaten[d][0].draw(dx, dy);
					break;
				}
				case HUMMAN:
				case HUMBUG:
				{
					int d = c & 3;
					int p = (c & HUMBUG) ? 1 : 0;
					if (d == 0) d = 4;
					d = d - 1;
					hman[d][p].draw(dx, dy);
					break;
				}
				case NHMMAN:
				case NHUMER:
				{
					int d = c & 3;
					int p = (c & NHMMAN) ? 1 : 0;
					if (d == 0) d = 4;
					d = d - 1;
					nhman[d][p].draw(dx, dy);
					break;

					break;
				}
				default:
				{
					path.draw(dx, dy);
					break;

				}

				}
				if (i == limitx) {
					stopper(0, 0, SPSIZE - delta, SPSIZE).draw(dx, dy);
				}
#ifdef _DEBUG4_
				//if ((xx % 2 == 0) && (yy % 2 == 0))
				{
					Debugfont(U"{},{}"_fmt(xx, yy)).draw(TextStyle::Shadow(shadowOffset, shadowColor), dx, dy);
					Debugfont(U"={}"_fmt(c)).draw(TextStyle::Shadow(shadowOffset, shadowColor), dx, dy + 14);
				}
#endif
			}
		}
		// Draw men.
		int t;
		if (f) t = 0;
		else t = 1;
		// Print << U"half:" << t;
		if (
			(playerstate == NORMAL) ||
			(playerstate == EXIT)
			)
		{
			man[nextDir][t].draw(5 * SPSIZE, 3 * SPSIZE);
		}
		// Rightside info
		int ly = 24 * 3, lx = 58 * 12;
		int s = getData().score;
		if (s < 0)
		{
			s = 0;
		}
		FontAsset(U"PC8001")(U"SCORE").draw(lx, ly);
		FontAsset(U"PC8001")(U"{:0>6}"_fmt(s)).draw(lx, ly + 24 * 2);
		FontAsset(U"PC8001")(U"TIME").draw(lx, ly + 24 * 6);
		FontAsset(U"PC8001")(U"{:0>2}"_fmt(getData().time)).draw(lx, ly + 24 * 8);
		FontAsset(U"PC8001")(U"ALIEN").draw(lx, ly + 24 * 10);
		FontAsset(U"PC8001")(U"{}"_fmt(getData().bugs)).draw(lx + 12 * 3, ly + 24 * 12);
	}

	~Game()
	{
		delete[] cbug;
	};
};

void Main()
{
	//Settings
	Window::SetTitle(U"BUG FIRE!");
	Scene::SetBackground(Palette::Black);
	// ウィンドウの閉じるボタンを押すか、System::Exit() を呼ぶのを終了操作に設定,
	// エスケープキーを押しても終了しないようになる
	System::SetTerminationTriggers(UserAction::CloseButtonClicked);

	// 拡大縮小時に最近傍法で補間  // default Texture::Linear
	//Scene::SetTextureFilter(TextureFilter::Nearest);

	FontAsset::Register(U"PC8001", 22, Resource( U"font/DotGothic16-Regular.ttf" ));
	//FontAsset::Register(U"PC8001", 22, Resource( U"font/PC-8001.ttf" ));
	FontAsset(U"PC8001").preload(U"　・！？．。（）［］｛｝＊／゛゜←→↑↓＜＝＞■ー０１２３４５６７８９ＡAｂＢＣCＥEＦＧＨＩIＬLMＭｎＮNＯOＰＲRＳSＴTＵＶＸｙＹアイウオカキクケコサシスセソタチッツテトナニネノハフヘホマムメモヤュラリルレロワヲン");
	s3d::ClearPrint();

	//Scene::SetResizeMode(ResizeMode::Keep);
	//Window::SetStyle(WindowStyle::Sizable);
	// ウィンドウの閉じるボタンを押すか、System::Exit() を呼ぶのを終了操作に設定,
	// エスケープキーを押しても終了しないようになる
	//System::SetTerminationTriggers(UserAction::CloseButtonClicked);

	App manager;

	manager.add<Title>(U"Title");
	manager.add<Title2>(U"Title2");
	manager.add<Instruction>(U"Instruction");
	manager.add<InitGame>(U"InitGame");   // ゲーム開始
	manager.add<InitScreen>(U"InitScreen"); // 面の初期化
	manager.add<StartGame>(U"StartGame");  // 中間報告の表示
	manager.add<Game>(U"Game");
	manager.add<GameOver>(U"GameOver");

    //manager.init(U"GameOver");

	Scene::SetResizeMode(ResizeMode::Keep);
	Window::SetStyle(WindowStyle::Sizable);

	while (System::Update())
	{
// "Licenses" ボタンが押されたら
		if (not manager.update())
		{
			break;
		}
	}
};
