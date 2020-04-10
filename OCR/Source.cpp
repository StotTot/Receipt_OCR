#include <iostream>
#include "leptonica/allheaders.h"
#include "tesseract/baseapi.h"
#include <fstream>
#include "pqxx/pqxx"
#include <string>

std::string receipt_path;
char* outText;
float total;
std::string contents;
void OCR_read(std::string datafile) {

    std::ofstream temp("temp.txt");
    
    receipt_path = "C:\\Receipts\\" + datafile;
    char* path = &receipt_path[0];
    tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
    //Initialize tesseract-ocr with English, without specifying tessdata path
    if (api->Init("C:\\tessdata", "eng")) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }


    // Open input image with leptonica library
    Pix* image = pixRead(path);
    api->SetImage(image);
    // Get OCR result
    outText = api->GetUTF8Text();
    printf("OCR output:\n%s", outText);

    if (temp.is_open()) {
        temp << outText;
    }
    else {
        std::cout << "Text file is not open!\n";
    }


    // Destroy used object and release memory
    api->End();
    delete api;
    delete[] outText;
    pixDestroy(&image);
}


int insert_DB(double total, std::string textFile) {
    //Database conneciton

    std::string sql;
    std::string user, pass, cline;
    std::string cred[2];
    std::ifstream credentials, data;

    int ccount = 0;
    credentials.open("C:\\OCR_cred.txt");
    while (std::getline(credentials, cline)) {
        cred[ccount] = cline;
        ccount++;
    }
    user = cred[0];
    pass = cred[1];
    std::string connstring = "dbname = OCR user = yourserveruser password = yourserverpass \
        hostaddr = 127.0.0.1 port = 5432";
    while (connstring.find("yourserveruser") != std::string::npos)
        connstring.replace(connstring.find("yourserveruser"), 14, user);
    while (connstring.find("yourserverpass") != std::string::npos)
        connstring.replace(connstring.find("yourserverpass"), 14, pass);




    try {
        pqxx::connection C(connstring);
        if (C.is_open()) {
            std::cout << "Opened database successfully: " << C.dbname() << std::endl;
        }
        else {
            std::cout << "Can't open database" << std::endl;
            return 1;
        }

        /* Create SQL statement */
        std::string str1("INSERT INTO public.\"RECEIPTS\"(\"PURCH_TOTAL\", \"FILE_NAME\", \"TEXT_DATA\") "   \
            "VALUES (value1, 'value2', 'value3');");

        std::string str2(std::to_string(total)); //convert total to a string

        data.open("temp.txt");

        contents.assign((std::istreambuf_iterator<char>(data)),
            (std::istreambuf_iterator<char>()));

        while (str1.find("value1") != std::string::npos)
            str1.replace(str1.find("value1"), 6, str2);
        while (str1.find("value2") != std::string::npos)
            str1.replace(str1.find("value2"), 6, textFile);
        while (str1.find("value3") != std::string::npos)
            str1.replace(str1.find("value3"), 6, contents);

        //std::cout << str1 << std::endl;

        sql = str1;

        /* Create a transactional object. */
        pqxx::work W(C);

        /* Execute SQL query */
        W.exec(sql);
        W.commit();
        std::cout << "Records created successfully" << std::endl;
        C.disconnect();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

}

float parse() {


    std::ifstream ifile("temp.txt");

    std::string line, token = "TOTAL ";
    //This currently finds both the subtotal and total line
    //TODO: Look for other lines
    while (std::getline(ifile, line)) {
        if (line.find(token) != std::string::npos) {
            std::cout << line << std::endl;
            // Find position of ':' using find() 
            int pos = line.find("TOTAL");

            // Copy substring after pos 
            std::string sub = line.substr(pos + 6);
            total = ::atof(sub.c_str());
        }
    }

    std::cout << "$" << total << std::endl;
    return total;
}

std::string print_UI(int x) {

    std::string textFile;

    if (x != 1)
    {


        std::cout << "****************************************\n";
        std::cout << "****************************************\n";
        std::cout << "****          Fredonia OCR          ****\n";
        std::cout << "****************************************\n";
        std::cout << "****************************************\n";
        std::cout << "\n";
        std::cout << "\n";
        std::cout << "Please put your receipts into C:\\Receipts\n";
        std::cout << "Once you do, hit any button to continue.\n";
        system("pause");
    }
    std::cout << "Please type the name of the receipt\n";
    std::cin >> textFile;

    return textFile;
}

int main() {
    int usrInpt = 0;
    do {
        std::string textFile = "";
        textFile = print_UI(usrInpt);
        OCR_read(textFile);
        total = parse();
        insert_DB(total, textFile);
        std::cout << "Receipt Scanned. Do you want to scan another?\n";
        std::cout << "1 to continue, -1 to quit\n";
        std::cin >> usrInpt;
    } while (usrInpt != -1);
    return 0;
}