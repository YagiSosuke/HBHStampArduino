/**
0 あかさ飲み読み込み
1 その他の文字追加、クラス化、関数化して見やすく修正
2 各パネル押下処理追加
3 csvで読み込みできるようにする
4 カード型NFCも読めるようにする(UIDの桁数が違う場合でも)
5 真ん中のボタンでパーツ切り替えができるようにする
6 カード読み込み時のcsvを、もう少しわかりやすくする（一行にパネルの部位をまとめる、部位を変えるときは改行する）
7-a 濁点、半濁点パネルを用意
いつか DMA転送で画像表示を素早くする
**/
#include <M5Stack.h>
#include <SPI.h>
#include <MFRC522.h>
#include "BluetoothSerial.h"

BluetoothSerial bts;
String type="Head";//ハンコのタイプ
String kana="";//ハンコのもじ

#define SS_PIN 21
#define RST_PIN 33

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

#define moji_a "04 15 99 EA F0 64 81"

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
bool img_sw=false;



//スタンプに読み込まれている文字を記憶するクラス
class StampWord{
  private:
  int Type;  //0あなまら, 1かさた, 2は, 3やわ
  int Boin;  //0~4あ~お, 5~9が～ご, 10~14ぱ～ぽ
  int Shiin; //0~9あ～わ

  public:
  //初期設定
  StampWord(){
    Type = 0;
    Boin = 0;
    Shiin = 0;  
  }
  //母音変更
  void BoinChange(long addNum){
    //addNumだけ母音を足す
    //0あなまら, 1かさた, 2は, 3やわ
    if(addNum > 0){
      switch(Type){
        case 0:
          if(Boin < 4){
            Boin += addNum;
          }else{
            Boin = 0;
          }
          break;
        case 1:
          if(Boin < 9){
            Boin += addNum;
          }else{
            Boin = 0;
          }
          break;
        case 2:
          if(Boin < 14){
            Boin += addNum;
          }else{
            Boin = 0;
          }
          break;  
        case 3:
          if(Boin < 2){
            Boin += addNum;
          }else{
            Boin = 0;
          }
          break;
      }  
    }else{
      switch(Type){
        case 0:
          if(Boin > 0){
            Boin += addNum;
          }else{
            Boin = 4;
          }
          break;
        case 1:
          if(Boin > 0){
            Boin += addNum;
          }else{
            Boin = 9;
          }
          break;
        case 2:
          if(Boin > 0){
            Boin += addNum;
          }else{
            Boin = 14;
          }
          break;  
        case 3:
          if(Boin > 0){
            Boin += addNum;
          }else{
            Boin = 2;
          }
          break;
      }  
    }
    Serial.println("母音変更");
    Serial.print("母音:");
    Serial.println(Boin);
    Serial.print("子音:");
    Serial.println(Shiin);
  }
  //子音変更
  void ShiinChange(int cardNum){  
    Shiin = cardNum;
    Boin = 0;
    
    //0あなまら, 1かさた, 2は, 3やわ
    //タイプ決定
    if(cardNum==0 || cardNum==4 || cardNum==6 || cardNum==8){
      Type = 0;
    }else if(cardNum==1 || cardNum==2 || cardNum==3){
      Type = 1;
    }else if(cardNum==5){
      Type = 2;
    }else{
      Type = 3;
    }
    Serial.println("子音変更");
    Serial.print("母音:");
    Serial.println(Boin);
    Serial.print("子音:");
    Serial.println(Shiin);
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
        }else{
          pass.concat("g");          
        }
        break;  
      case 2:
        if(Boin < 5){
          pass.concat("s");
        }else{
          pass.concat("z");          
        }
        break;  
      case 3:
        if(Boin < 5){
          pass.concat("t");
        }else{
          pass.concat("d");          
        }
        break;  
      case 4:
        pass.concat("n");
        break;  
      case 5:
        if(Boin < 5){
          pass.concat("h");
        }else if(Boin < 10){
          pass.concat("b");          
        }else{
          pass.concat("p");
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
    }
    //母音を設定
    if(Shiin!=7 && Shiin!=9){
      if(Boin==0 || Boin==5 || Boin==10){
        pass.concat("a");
      }else if(Boin==1 || Boin==6 || Boin==11){
        pass.concat("i");
      }else if(Boin==2 || Boin==7 || Boin==12){
        pass.concat("u");
      }else if(Boin==3 || Boin==8 || Boin==13){
        pass.concat("e");
      }else if(Boin==4 || Boin==9 || Boin==14){
        pass.concat("o");
      }
    }

    pass.concat(".png");
    
    return pass;
  }
};

//String wordIDlist[10] = {"04 15 99 EA F0 64 81", "04 19 99 EA F0 64 81", "04 1D 99 EA F0 64 81", "04 21 99 EA F0 64 81", "04 25 99 EA F0 64 81", "04 29 99 EA F0 64 81", "04 2D 99 EA F0 64 81", "04 31 99 EA F0 64 81", "04 35 99 EA F0 64 81", "04 39 99 EA F0 64 81"};//カードIDの配列
String imglist[10] = {"a.png", "ka.png", "sa.png", "ta.png", "na.png", "ha.png", "ma.png", "ya.png", "ra.png", "wa.png"};//画像ファイルのアドレス
int key_num = 10;//idと画像の数
/*
String panelIDlist[15] = {"04 3D 99 EA F0 64 81", "04 41 99 EA F0 64 81", "04 45 99 EA F0 64 81", "04 49 99 EA F0 64 81", "04 4D 99 EA F0 64 81",
                          "04 51 99 EA F0 64 81", "04 54 9A EA F0 64 81", "04 58 9A EA F0 64 81", "04 5C 9A EA F0 64 81", "04 60 9A EA F0 64 81",
                          "04 64 9A EA F0 64 81", "04 68 9A EA F0 64 81", "04 78 9A EA F0 64 81", "04 70 9A EA F0 64 81", "04 74 9A EA F0 64 81"
                          };//カードIDの配列
                          */
char buf[100];//画像ファイル読み出しpath用のバッファ

String strUID;//UID記憶用変数

bool wordReadF;//読み込んだものが言葉かどうか

File csvFile;//UIDのcsvファイル

//スタンプに読み込まれている文字を記憶するクラス
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
    Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
    dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
    bts.begin("M5Stack");//PC側で確認するときの名前

    stampWord = StampWord();
    
    M5.Lcd.drawPngFile(SD, "/hiragana/a.png", 80, 0, 240, 240, 40, 0);//画像の読み込み
    M5.Lcd.drawPngFile(SD, "/Parts/Head.png", 0, 0);//画像の読み込み
}

void loop() {
  M5.update();
  
  //画像変更処理
  if(img_sw){
    //bts.print(type);
    //bts.print(",");
    bts.println(kana);
    //bts.println(buf);
    M5.Lcd.drawPngFile(SD, buf, 80, 0, 240, 240, 40, 0);//画像の読み込み
    img_sw=false;//画像読込スイッチをoff
  }
  
  
  ButtonPush();
  
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  } 
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
 
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
    //bts.println(strUID);
  }else if(bufLength == 7){
    //UID長い時は下のコードみたいに伸ばしておく
    strUID = strBuf[0] + " " + strBuf[1] + " " + strBuf[2] + " " + strBuf[3] + " " + strBuf[4] + " " + strBuf[5] + " " + strBuf[6];
    Serial.println("bufLen = 7 ");
    Serial.println(strUID);
    //bts.println(strUID);
  }
  //UID短い時は上のコード
  //strUID = strBuf[0] + " " + strBuf[1] + " " + strBuf[2] + " " + strBuf[3];
  //UID長い時は下のコードみたいに伸ばしておく
  //strUID = strBuf[0] + " " + strBuf[1] + " " + strBuf[2] + " " + strBuf[3] + " " + strBuf[4] + " " + strBuf[5] + " " + strBuf[6];
  //Serial.println(strUID);

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

//文字を読み取る関数
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
      //UIDがUID1と一致した際の処理を書く
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
      stampWord.ShiinChange(i);//母音変更を記憶
      
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
      break;
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
        bts.println(type);
        M5.Lcd.drawPngFile(SD, "/Parts/Body.png", 0, 0);//画像の読み込み
    }else if(type == "Body"){
        type = "Hip";
        bts.println(type);
        M5.Lcd.drawPngFile(SD, "/Parts/Hip.png", 0, 0);//画像の読み込み
    }else if(type == "Hip"){
        type = "Head";        
        bts.println(type);
        M5.Lcd.drawPngFile(SD, "/Parts/Head.png", 0, 0);//画像の読み込み
    }    
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
