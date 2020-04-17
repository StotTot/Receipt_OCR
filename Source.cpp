#include <iostream>
#include "leptonica/allheaders.h"
#include "tesseract/baseapi.h"
#include <fstream>
#include "pqxx/pqxx"
#include <string>
#include <regex>
#include <filesystem>
 
char* outText;
float total;
std::string dt, temp_dt, contents, receipt_path;

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


bool tableExist(std::string connstring) {

    try {
        pqxx::connection C(connstring);
        if (C.is_open()) {
            std::cout << "Opened database successfully: " << C.dbname() << std::endl;
        }
        else {
            std::cout << "Can't open database" << std::endl;
            return 1;
        }

        //create table
        std::string test("CREATE TABLE IF NOT EXISTS public.\"RECEIPTS\"("  \
            "\"PURCH_ID\" SERIAL NOT NULL ," \
            "\"PURCH_TOTAL\" money," \
            "\"PURCH_DATE\" date," \
            "\"FILE_NAME\" name COLLATE pg_catalog.\"default\"," \
            "\"TEXT_DATA\" text COLLATE pg_catalog.\"default\"," \
            "CONSTRAINT \"RECEIPTS_pkey\" PRIMARY KEY(\"PURCH_ID\"));");
        //TODO: remove error statement everytime the table attempts to be created.

        pqxx::work T(C);

        /* Execute SQL query */

        T.exec(test);
        T.commit();
        C.disconnect();

    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}

int insert_DB(double total, std::string date, std::string textFile) {
    //Database connection

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
    
    tableExist(connstring);



    try {
        pqxx::connection C(connstring);
        if (C.is_open()) {
            std::cout << "Opened database successfully: " << C.dbname() << std::endl;
        }
        else {
            std::cout << "Can't open database" << std::endl;
            return 1;
        }
        
        std::string str1("INSERT INTO public.\"RECEIPTS\"(\"PURCH_TOTAL\", \"PURCH_DATE\", \"FILE_NAME\", \"TEXT_DATA\") "   \
            "VALUES (value1, 'value2', 'value3', 'value4');");

        std::string str2(std::to_string(total)); //convert total to a string

        data.open("temp.txt");

        contents.assign((std::istreambuf_iterator<char>(data)),
            (std::istreambuf_iterator<char>()));

        while (str1.find("value1") != std::string::npos)
            str1.replace(str1.find("value1"), 6, str2);
        while (str1.find("value2") != std::string::npos)
            str1.replace(str1.find("value2"), 6, date);
        while (str1.find("value3") != std::string::npos)
            str1.replace(str1.find("value3"), 6, textFile);
        while (str1.find("value4") != std::string::npos)
            str1.replace(str1.find("value4"), 6, contents);

        //std::cout << str1 << std::endl;

        sql = str1;

        pqxx::work W(C);
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

float parse_total() {


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
    ifile.close();
    std::cout << "$" << total << std::endl;
    return total;
}

std::string parse_date() {

    std::regex d("[0-9]{2}/[0-9]{2}/[0-9]{2}");
    std::smatch m;
    std::ifstream ifile("temp.txt");

    std::string line;
    //This currently finds both the subtotal and total line

    while (std::getline(ifile, line)) {
        if (std::regex_search(line, m, d)) {
            for (auto x : m) std::cout << x << " ";
            std::cout << std::endl;
            temp_dt = m.str();
            
            return temp_dt;
        }
    } 
    temp_dt = "Couldn't find the date";
    return temp_dt;
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
        //std::cout << "Once you do, hit any button to continue.\n";
        system("pause");
    }
    std::cout << "Please type the name of the receipt with extension.\n";
    std::cin >> textFile;

    return textFile;
}


int main() {
    int usrInpt = 0;
    do {
        std::string textFile = "";
        textFile = print_UI(usrInpt);
        OCR_read(textFile);
        total = parse_total();
        dt = parse_date();
        std::cout << temp_dt << std::endl;
        dt = temp_dt.substr(0, 8);
        std::cout << dt << std::endl;
        insert_DB(total, dt, textFile);
        std::cout << "Receipt Scanned. Do you want to scan another?\n";
        std::cout << "1 to continue, -1 to quit\n";
        std::cin >> usrInpt;
    } while (usrInpt != -1);
    return 0;
}