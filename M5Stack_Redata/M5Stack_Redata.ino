/**
0 あかさ飲み読み込み
1 その他の文字追加、クラス化、関数化して見やすく修正
2 各パネル押下処理追加
3 csvで読み込みできるようにする
4 カード型NFCも読めるようにする(UIDの桁数が違う場合でも)
5 真ん中のボタンでパーツ切り替えができるようにする
6 カード読み込み時のcsvを、もう少しわかりやすくする（一行にパネルの部位をまとめる、部位を変えるときは改行する）
7-a がざだばぱパネルを用意
いつか DMA転送で画像表示を素早くする
**/
#include <M5Stack.h>
#include <SPI.h>
#include <MFRC522.h>
#include "BluetoothSerial.h"

//DMA転送に必要なファイル
#pragma GCC optimize ("O3")
//#include <M5StackUpdater.h>     // https://github.com/tobozo/M5Stack-SD-Updater/
#include <esp_heap_alloc_caps.h>
#include <vector>
#include "src/MainClass.h"
#include "src/DMADrawer.h"
MainClass main;
std::vector<const uint8_t*> fbuf;  //画像情報
std::vector<int32_t> fbufsize;     //サイズ情報
//ここまで - DMA転送に必要なファイル

BluetoothSerial bts;
String type="Head";//ハンコのタイプ
String kana="";//ハンコのもじ

//ピン
#define SS_PIN 21
#define RST_PIN 33

//トーン関数で使用する
#define NOTE_DL1 147
#define NOTE_DL2 165
#define NOTE_DL3 175
#define NOTE_DL4 196
#define NOTE_DL5 221
#define NOTE_DL6 248
#define NOTE_DL7 278

#define NOTE_DH1 589
#define NOTE_DH2 661
#define NOTE_DH3 700
#define NOTE_DH4 786
#define NOTE_DH5 882
#define NOTE_DH6 990
#define NOTE_DH7 112

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
bool img_sw=false;

//String imglist[15] = {"a.png", "ka.png", "sa.png", "ta.png", "na.png", "ha.png", "ma.png", "ya.png", "ra.png", "wa.png", "ga.png", "za.png", "da.png", "ba.png", "pa.png"};//画像ファイルのアドレス
// ここで画像ファイルのディレクトリ名を指定する
std::vector<String> imageDirs = {
  "/_hiragana/a_line", "/_hiragana/k_line", "/_hiragana/s_line", "/_hiragana/t_line", "/_hiragana/n_line", 
  "/_hiragana/h_line", "/_hiragana/m_line", "/_hiragana/y_line", "/_hiragana/r_line", "/_hiragana/w_line", 
  "/_hiragana/g_line", "/_hiragana/z_line", "/_hiragana/d_line", "/_hiragana/b_line", "/_hiragana/p_line"};
int key_num = 17;//idと画像の数

char buf[100];//画像ファイル読み出しpath用のバッファ

String strUID;//UID記憶用変数

bool wordReadF;//読み込んだものが言葉かどうか

File csvFile;//UIDのcsvファイル

bool pushF = false;
bool showablePushTextF = false;

//DMAで使用 - 画像が読み込まれているディレクトリを開く
bool loadImages(const String& path)
{
  //現在読み込まれているメモリを開放する
  bool res = false;
  if (!fbuf.empty()) {
    for (int i = 0; i < fbuf.size(); i++) free(const_cast<uint8_t*>(fbuf[i]));
  }
  fbuf.clear();
  fbufsize.clear();

  //メモリに画像を読み込む
  Serial.println(path);
  File root = SD.open(path);
  File file = root.openNextFile();
  uint8_t* tmp;
  while (file) {
    tmp = (uint8_t*)pvPortMallocCaps(file.size(), MALLOC_CAP_DEFAULT);
    if (tmp > 0) {
      file.read(tmp, file.size());
      fbufsize.push_back(file.size());
      fbuf.push_back(tmp);
      res = true;
    }
    file = root.openNextFile();
  }
  return res;
}

//スタンプに読み込まれている文字を管理するクラス
class StampWord{
  public:
  int Boin;  //0~4あ~お
  int Shiin; //0~9あ～わ, 10が, 11ざ, 12だ, 13ば, 14ぱ, 15゛, 16゜
  int beforeShiin;

  //初期設定
  StampWord(){
    Boin = 0;
    Shiin = 0; 
    beforeShiin = 0; 
  }
  //母音変更
  void BoinChange(long addNum){
    int boinDataNum;
    if(Shiin == 7 || Shiin == 9){
      boinDataNum = boinDataNum = 3;
    }else{
      boinDataNum = 5;
    }

    if(addNum > 0){
      Boin = (Boin+1) % boinDataNum;
    }else{
      Boin = (Boin==0) ? boinDataNum-1 : Boin-1;
    }
  }
  //子音変更
  void ShiinChange(int cardNum){
    beforeShiin = Shiin;
    Shiin = cardNum;
    if(Shiin != 15 && Shiin != 16){
      Boin = 0;
    }
  }
  //画像のパスを返す
  String ReturnPass(){
    String pass = "";
    Serial.println("パス返す");
    Serial.print("母音:");
    Serial.println(Boin);
    Serial.print("子音:");
    Serial.println(Shiin);
    
    //子音を設定
    switch(Shiin){
      case 0:
        break;  
      case 1:
        if(Boin < 5){
          pass.concat("k");         
        }
        break;  
      case 2:
        if(Boin < 5){
          pass.concat("s");        
        }
        break;  
      case 3:
        if(Boin < 5){
          pass.concat("t");       
        }
        break;  
      case 4:
        pass.concat("n");
        break;  
      case 5:
        if(Boin < 5){
          pass.concat("h");
        }
        break;  
      case 6:
        pass.concat("m");
        break;  
      case 7:
        switch(Boin){
          case 0:
            pass.concat("ya");
            break;
          case 1:
            pass.concat("yu");
            break;
          case 2:
            pass.concat("yo");
            break;  
        }
        break;  
      case 8:
        pass.concat("r");
        break;  
      case 9:
        switch(Boin){
          case 0:
            pass.concat("wa");
            break;
          case 1:
            pass.concat("wo");
            break;
          case 2:
            pass.concat("nn");
            break;  
        }
        break;
        case 10:
          pass.concat("g");
          break;
        case 11:
          pass.concat("z");
          break;
        case 12:
          pass.concat("d");
          break;
        case 13:
          pass.concat("b");
          break;
        case 14:
          pass.concat("p");
          break;
        case 15:
          switch(beforeShiin){
            case 1:
              Shiin = 10;
              pass.concat("g");
              break;
            case 2:
              Shiin = 11;
              pass.concat("z");
              break;
            case 3:
              Shiin = 12;
              pass.concat("d");
              break;
            case 5: 
              Shiin = 13;
              pass.concat("b");
              break; 
            case 10: 
              Shiin = 1;
              pass.concat("k");
              break; 
            case 11: 
              Shiin = 2;
              pass.concat("s");
              break; 
            case 12: 
              Shiin = 3;
              pass.concat("t");
              break; 
            case 13: 
              Shiin = 5;
              pass.concat("h");
              break; 
            case 14: 
              Shiin = 13;
              pass.concat("b");
              break; 
          }
          break;
        case 16:
          switch(beforeShiin){
            case 5: 
              Shiin = 14;
              pass.concat("p");
              break; 
            case 13: 
              Shiin = 14;
              pass.concat("p");
              break; 
            case 14: 
              Shiin = 5;
              pass.concat("h");
              break; 
          }
          break;
    }
    //母音を設定
    if(Shiin!=7 && Shiin!=9){
      switch (Boin)
      {
        case 0:
          pass.concat("a");
          break;
        case 1:
          pass.concat("i");
          break;
        case 2:
          pass.concat("u");
          break;
        case 3:
          pass.concat("e");
          break;
        case 4:
          pass.concat("o");
          break;
      }
    }

    pass.concat(".png");
    
    return pass;
  }
};
StampWord stampWord;


/*
 * Initialize.
 */
void setup() {
  M5.begin(); 

  Serial.begin(115200);         // Initialize serial communications with the PC
  while (!Serial);            // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();                // Init SPI bus

  mfrc522.PCD_Init();         // Init MFRC522 card
                              //バッテリ―残量が読めない原因
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
  bts.begin("M5Stack");//PC側で確認するときの名前
  
  stampWord = StampWord();
  //DMA転送に必要な処理
#ifdef __M5STACKUPDATER_H
  if(digitalRead(BUTTON_A_PIN) == 0) {
     Serial.println("Will Load menu binary");
     updateFromFS(SD);
     ESP.restart();
  }
#endif

  main.setup(&M5.Lcd);

  fbuf.clear();
  fbufsize.clear();
  loadImages(imageDirs[0]);

  M5.Lcd.drawJpg(fbuf[0], fbufsize[0], 80, 0, 240, 240, 40, 0);//画像の読み込み
  M5.Lcd.drawJpgFile(SD, "/Parts/Head.jpg", 0, 0);//画像の読み込み
  //ここまで - DMA転送に必要な処理

}

void loop() {
  M5.update();
  
  //画像変更処理
  if(img_sw){
    //bts.print(type);
    //bts.print(",");
    bts.println(kana);
    //bts.println(buf);
    //M5.Lcd.drawPngFile(SD, buf, 80, 0, 240, 240, 40, 0);//画像の読み込み
    M5.Lcd.drawJpg(fbuf[stampWord.Boin], fbufsize[stampWord.Boin], 80, 0, 240, 240, 40, 0);       // <= drawJpg (DMA transfer)
    img_sw=false;//画像読込スイッチをoff
  }
  
  
  ButtonPush();
  SerialRead();


  // Select one of the cards
  if (mfrc522.PICC_ReadCardSerial()) {
    pushF = true;
    showablePushTextF = true;

    String strBuf[mfrc522.uid.size];
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      strBuf[i] =  String(mfrc522.uid.uidByte[i], HEX);  // (E)using a constant integer
      if(strBuf[i].length() == 1){  // 1桁の場合は先頭に0を追加
        strBuf[i] = "0" + strBuf[i];
      }
    }

    //読み込んだUIDを変数に入れる処理
    int bufLength = sizeof(strBuf) / sizeof(strBuf[0]);
    if(bufLength == 4){
      strUID = strBuf[0] + " " + strBuf[1] + " " + strBuf[2] + " " + strBuf[3];
      Serial.println("bufLen = 4 ");
      Serial.println(strUID);
    }else if(bufLength == 7){
      //UID長い時は下のコードみたいに伸ばしておく
      strUID = strBuf[0] + " " + strBuf[1] + " " + strBuf[2] + " " + strBuf[3] + " " + strBuf[4] + " " + strBuf[5] + " " + strBuf[6];
      Serial.println("bufLen = 7 ");
      Serial.println(strUID);
    }

    wordReadF = false;
    
    //言葉を読んだ時、以下を実行
    csvFile = SD.open("/UID/word.csv");
    ReadWord();  
    csvFile.close();
    
    if(!wordReadF){
      //パネルを読んだ時、以下を実行
      csvFile = SD.open("/UID/panel.csv");
      PanelPush();
      csvFile.close();
    }  
    return;
  }
 
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    if(!pushF && showablePushTextF){
      bts.println("notNewPanelRead");
      showablePushTextF = false;
    }
    pushF = false;
    return;
  } 
}


//SDカードからWordUIDを読み取る
String SDreadWord(){
  int num;
  String UID = "";
  
  while((char)(num = csvFile.read()) != ',' && num > 0){
    UID.concat((char)num);
  }

  return UID;
}

//SDカードからPanelUIDを読み取る
String SDreadPanel(bool *panelNoAddF){
  int num;
  String UID = "";
  while((char)(num = csvFile.read()) != ',' && (char)num != '\r' && num > 0){
    UID.concat((char)num);
  }
  if((char)num == '\r' || num <= 0){
    //panelNoを増やすフラグを立てる  
    (*panelNoAddF) = true;
    csvFile.read();             //次の文字コード読み込み時に改行コードが読まれないように、ここで読んでおく
  }

  return UID;  
}

//カードで文字を読み取る関数
void ReadWord(){
  String Word = "";
  Serial.print("strUID = ");
  Serial.println(strUID);
  for(int i=0;i<key_num;i++){
    Word = "";
    Word.concat(SDreadWord());
    Serial.print(i);
    Serial.print(" SD = ");
    Serial.println(Word);
    //SDカードからサンプルのUIDを読み込み、現在読み込んだUIDと比較
    if (strUID.equalsIgnoreCase(Word) ){  // 大文字小文字関係なく比較
      Serial.print("カードのUID = ");
      Serial.println(i);
      //UIDがUID1と一致した際の処理を書く
      if(!((i == 15 && (stampWord.Shiin == 0 || stampWord.Shiin == 4 || stampWord.Shiin == 6 || stampWord.Shiin == 7 || stampWord.Shiin == 8 || stampWord.Shiin == 9)) || 
         (i == 16 && (stampWord.Shiin != 5 && stampWord.Shiin != 13 && stampWord.Shiin != 14)))){
        Serial.print("UID: ");
        Serial.println(Word);
        //ブザーの再生
        M5.Speaker.tone(NOTE_DH2, 50); 
        delay(100);
        M5.Speaker.tone(NOTE_DH3, 50); 
        delay(100);
        M5.Speaker.tone(NOTE_DH4, 50); 
        delay(100);
        M5.Speaker.tone(NOTE_DH5, 50);
        stampWord.ShiinChange(i);//子音変更を記憶
        
        //パスの生成 
        kana = stampWord.ReturnPass();
        Serial.println(stampWord.ReturnPass());
        String imgPass = "/hiragana/";
        imgPass.concat(kana);
        imgPass.toCharArray(buf,imgPass.length()+1);//パスをchar型に変換
        img_sw=true;//画像読込スイッチをOn
        Serial.println(buf);//アドレスの確認
        bts.println("CardRead");
        delay(20);
        wordReadF = true;

        loadImages(imageDirs[stampWord.Shiin]);
        break;
      }
    }else{
      // もし登録されていないNFCなら情報を表示
      //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
    }
  }
}

//スタンプ押下場所を読み取る関数
void PanelPush(){
  int panelNo = 0;
  String panel = "";
  bool panelNoAddF = false;
  while(panelNo<15){
    panel = SDreadPanel(&panelNoAddF);

    /*
    Serial.println(strUID);
    strUID.toCharArray(buf,strUID.length()+1);
    Serial.println("end");
    Serial.println(panel);
    panel.toCharArray(buf,panel.length()+1);
    Serial.println("end");
    Serial.print("比較すると");
    Serial.println(strUID.equalsIgnoreCase(panel));
    Serial.println();    
    */
    
    if (strUID.equalsIgnoreCase(panel)) {
      Serial.print("パネル押下:");
      Serial.print(panelNo % 5);
      Serial.print(',');
      Serial.println(panelNo / 5);
      bts.print(panelNo % 5);
      bts.print(',');
      bts.println(panelNo / 5);
      delay(20);
      break;
    }
    //panelNoを変える
    if(panelNoAddF){
      panelNo++;
      panelNoAddF = false;
    }
  }
}

//ボタンを押した時の処理
void ButtonPush(){
  if (M5.BtnC.wasPressed()) {
    stampWord.BoinChange(-1);  //母音を減らす
    //ブザーの再生
    M5.Speaker.tone(NOTE_DH2, 50); 

    //パスの生成
    kana = stampWord.ReturnPass();
    Serial.println(stampWord.ReturnPass());
    String imgPass = "/hiragana/";
    imgPass.concat(kana);
    imgPass.toCharArray(buf,imgPass.length()+1);//パスをchar型に変換
    img_sw=true;//画像読込スイッチをOn
    Serial.println(buf);//アドレスの確認
    M5.Lcd.drawJpgFile(SD, buf); //SDカードから画像の読み込み
    delay(20);
  }
  if (M5.BtnA.wasPressed()) {    
    stampWord.BoinChange(1);  //母音を増やす
    //ブザーの再生
    M5.Speaker.tone(NOTE_DH2, 50); 

    //パスの生成
    kana = stampWord.ReturnPass();
    Serial.println(stampWord.ReturnPass());
    String imgPass = "/hiragana/";
    imgPass.concat(kana);
    imgPass.toCharArray(buf,imgPass.length()+1);//パスをchar型に変換
    img_sw=true;//画像読込スイッチをOn
    Serial.println(buf);//アドレスの確認
    M5.Lcd.drawJpgFile(SD, buf); //SDカードから画像の読み込み
    delay(20);
  }

  //真ん中のボタンを押すとスタンプのタイプが変わる
  if (M5.BtnB.wasPressed()) {
    M5.Speaker.tone(NOTE_DH2, 50);
    
    if(type == "Head"){
      type = "Body";
      M5.Lcd.drawJpgFile(SD, "/Parts/Body.jpg", 0, 0);//画像の読み込み
    }else if(type == "Body"){
      type = "Hip";
      M5.Lcd.drawJpgFile(SD, "/Parts/Hip.jpg", 0, 0);//画像の読み込み
    }else if(type == "Hip"){
      type = "Head";
      M5.Lcd.drawJpgFile(SD, "/Parts/Head.jpg", 0, 0);//画像の読み込み
    }    
    bts.println(type);

    bts.print("Power printable: ");
    bts.println(M5.Power.canControl());
  }
}


//シリアル通信で送られてきたコマンドを読み込む巻数
void SerialRead() {

  int i=0;
  char read_c;                       //1文字読み込み
  char read_datas[15];               //受信文字列
  //データ受信
  if (bts.available()) {          //受信データがある時
    while (1) {
      if (bts.available()) {
        read_c = bts.read();      //1文字読み込み
        read_datas[i] = read_c;
        if (read_c == 'z'){          // 文字列の終わりは'z'で判断
          break; 
        }                            //zがあるとループから抜け出す
        i++;
      }
    }
    read_datas[i] = '¥0';            //受信データの最後にEOFを付加
    DataAnalysis(read_datas);        //データを解析する
  }
}
//データを分析する処理
void DataAnalysis(char read_datas[15]){
    //データ解析////////////////////////////////////////////////////////
    
    //現在読まれている部位と文字を返す
    if(read_datas[0]=='N' && read_datas[1]=='o' && read_datas[2]=='w' && read_datas[3]=='D' && read_datas[4]=='a' && read_datas[5]=='t' && read_datas[6]=='a'){
      bts.println(type);
      bts.println(kana);
    }
}



unsigned long getID(){
  if ( ! mfrc522.PICC_ReadCardSerial()) { //Since a PICC placed get Serial and continue
    return -1;
  }
  unsigned long hex_num;
  hex_num =  mfrc522.uid.uidByte[0] << 24;
  hex_num += mfrc522.uid.uidByte[1] << 16;
  hex_num += mfrc522.uid.uidByte[2] <<  8;
  hex_num += mfrc522.uid.uidByte[3];
  mfrc522.PICC_HaltA(); // Stop reading
  return hex_num;
}
 
/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < 15; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
