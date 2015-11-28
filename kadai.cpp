#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <string> 
#include <iostream> 
#include <curl/curl.h>
#include <time.h>
#include <fstream>
#include <mysql/mysql.h>

using namespace std;

int size_of_face = 0;
vector<string> path;
bool state;

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}

class Proc
{
    const char* MY_HOSTNAME;
    const char* MY_DATABASE;
    const char* MY_USERNAME;
    const char* MY_PASSWORD;
    const char* MY_SOCKET;
    enum {
        MY_PORT_NO = 3306,
        MY_OPT     = 0
    };
    MYSQL     *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

public:
    Proc();           // Constructor
    bool execMain();  // Main Process
};

/*
 * Proc - Constructor
 */
Proc::Proc()
{
    // Initialize constants
    MY_HOSTNAME = "localhost";
    MY_DATABASE = "opencv";
    MY_USERNAME = "root";
    MY_PASSWORD = "060510shiba";
    MY_SOCKET   = NULL;
}

/*
 * Main Process
 */
bool Proc::execMain()
{
    try {
        // Format a MySQL object
        conn = mysql_init(NULL);

        // Establish a MySQL connection
        if (!mysql_real_connect(
                conn,
                MY_HOSTNAME,
                MY_USERNAME,
                MY_PASSWORD,
                MY_DATABASE,
                MY_PORT_NO,
                MY_SOCKET, MY_OPT)
        ) {
            cerr << mysql_error(conn) << endl;
            return false;
        }

        // Execute a sql statement
        if (mysql_query(conn, "SELECT path FROM image")) {
            cerr << mysql_error(conn) << endl;
            return false;
        }

        // Get a result set
        res = mysql_use_result(conn);

        // Fetch a result set
        cout << "* MySQL - SELECT token FROM image in `"
             << MY_DATABASE << "`" << endl;
        while ((row = mysql_fetch_row(res)) != NULL){
            // cout << row[0] << endl;
            path.push_back(row[0]);
        }

        // Release memories
        mysql_free_result(res);

        // Close a MySQL connection
        mysql_close(conn);
    } catch (char *e) {
        cerr << "[EXCEPTION] " << e << endl;
        return false;
    }
    return true;
}


size_t callbackWrite(char *ptr, size_t size, size_t nmemb, string *stream)
{
        int dataLength = size * nmemb;
        stream->append(ptr, dataLength);
        return dataLength;
}

bool http_s3(string file_name){
        CURL *curl;
        CURLcode res;
        struct curl_httppost *post = NULL;
        struct curl_httppost *last = NULL;

        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        string chunk;

        // curl初期化のエラー処理
        if (curl == NULL) {
                cerr << "curl_easy_init() failed" << endl;
                return false;
        }

        // urlとpathの設定
        string path = "path=" + file_name;
        string url = "http://localhost:8000/s3?" + path;

        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            /* example.com is redirected, so we tell libcurl to follow redirection */ 
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
         
            /* Perform the request, res will get the return code */ 
            res = curl_easy_perform(curl);
            /* Check for errors */ 
            if(res != CURLE_OK)
              fprintf(stderr, "curl_easy_perform() failed: %s\n",
                      curl_easy_strerror(res));
         
            /* always cleanup */ 
            curl_easy_cleanup(curl);
          }
        return true;
}

bool http_communication(string file_name){
        CURL *curl;
        CURLcode ret;
        struct curl_httppost *post = NULL;
        struct curl_httppost *last = NULL;

        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        string chunk;

        // curl初期化のエラー処理
        if (curl == NULL) {
                cerr << "curl_easy_init() failed" << endl;
                return false;
        }

        // urlとpathの設定
        string url = "http://localhost:8000/mail";
        string path = "path=" + file_name;

        // パラメータ設定
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callbackWrite);
        // curl_formadd(&post, &last,
        //     CURLFORM_COPYNAME, "file",
        //     CURLFORM_FILECONTENT, file_name.c_str(), 
        //     CURLFORM_END
        // );
        // curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
        
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, path.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
        ret = curl_easy_perform(curl);
        curl_formfree(post);
        curl_easy_cleanup(curl);
        curl_global_cleanup();

        // http通信のエラー処理
        if (ret != CURLE_OK) {
                cerr << "curl_easy_perform() failed." << endl;
                return false;
        }

        cout << chunk << endl;
        return true;
}

int main()
{   
    // 1. load classifier
    std::string cascadeName = "/usr/local/share/OpenCV/haarcascades/haarcascade_frontalface_alt.xml"; //Haar-like
    cv::CascadeClassifier cascade;
    if(!cascade.load(cascadeName)){
        printf("ERROR: cascadeFile not found\n");
        return -1;
    }
      
    // 2. initialize VideoCapture
    cv::Mat frame, mosaic;
    cv::VideoCapture cap;
    cap.open(0);
    cap >> frame;
      
    // 3. prepare window and trackbar
    // cv::namedWindow("result", 1);

    double scale = 4.0;
    cv::Mat gray, smallImg(cv::saturate_cast<int>(frame.rows/scale),
                   cv::saturate_cast<int>(frame.cols/scale), CV_8UC1);

    // 画像顔が認識されるまで待つ。顔が認識されたらループを抜ける
    for(;;){
        
        // 4. capture frame
        cap >> frame;
        //convert to gray scale
        cv::cvtColor( frame, gray, CV_BGR2GRAY );
        
        // 5. scale-down the image
        cv::resize(gray, smallImg, smallImg.size(), 0, 0, cv::INTER_LINEAR);
        cv::equalizeHist(smallImg, smallImg);
        
        // 6. detect face using Haar-classifier
        std::vector<cv::Rect> faces;
        ///multi-scale face searching
        // image, size, scale, num, flag, smallest rect
        cascade.detectMultiScale(smallImg, faces, 1.1, 2, CV_HAAR_SCALE_IMAGE, cv::Size(30, 30));

        // 7. get face-region
        int i;
        for(i=0;i<faces.size();++i){
            cv::Point center;
            int radius;
            center.x = cv::saturate_cast<int>((faces[i].x + faces[i].width*0.5)*scale);
            center.y = cv::saturate_cast<int>((faces[i].y + faces[i].height*0.5)*scale);
            radius = cv::saturate_cast<int>((faces[i].width + faces[i].height)*0.25*scale);
            //mosaic
            if(size_of_face < 1) size_of_face = 1;
            cv::Rect roi_rect(center.x-radius, center.y-radius, radius*2, radius*2);
            mosaic = frame(roi_rect);
        }
        
        // 8. show mosaiced image to window
        // cv::imshow("result", frame );

        cv::waitKey(10);
        if (faces.size() > 0)
        {
            break;
        }
    }
    
    // 現在の時間を取得する
    time_t now = time(NULL);
    struct tm *pnow = localtime(&now);
    char s[19];
    sprintf(s, "%2d%2d%d-%d%d%d", pnow->tm_year + 1900, pnow->tm_mon + 1, pnow->tm_mday, pnow->tm_hour, pnow->tm_min, pnow->tm_sec);
    string datetime(s);

    // 顔の画像を出力する
    const string file_name = "/usr/local/var/www/htdocs/opencv-kadai/storage/app/" + datetime + ".jpg";
    if(cv::imwrite(file_name, mosaic)){
        cout << "imwrite:" << file_name << " ... success" << endl;
    }else{
        cout << "imwrite:" << file_name << " ... failure" << endl;
    }

    // データベースからファイルのパスを受け取る
    // これをテストデータとして以下作業をしていく
    try {
        Proc objMain;
        bool bRet = objMain.execMain();
        if (!bRet) cout << "MysqlConnection ERROR!" << endl;
    } catch (char *e) {
        cerr << "Mysql EXCEPTION " << e << endl;
        return 1;
    }

    // ここでデータベースにデータを保存する
    if (!http_s3(datetime))
    {
        perror("HTTP COMMUNICATION Failed....\n");
        return -1;
    }

    return 0;
}