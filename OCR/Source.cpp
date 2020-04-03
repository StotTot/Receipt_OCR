#include <iostream>
#include "../../../../../../vcpkg-master/installed/x64-windows/include/leptonica/allheaders.h"
#include "../../../../../../vcpkg-master/installed/x64-windows/include/tesseract/baseapi.h"
#include <fstream>
#include <../../../../../../vcpkg-master/installed/x64-windows/include/pqxx/pqxx>
#include <string>



float total;
void OCR_read() {

    std::ofstream temp("temp.txt");
    
    char* outText;

    tesseract::TessBaseAPI* api = new tesseract::TessBaseAPI();
    // Initialize tesseract-ocr with English, without specifying tessdata path
    if (api->Init(NULL, "eng")) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        exit(1);
    }

    // Open input image with leptonica library
    Pix* image = pixRead("/path to image/");
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


int insert_DB(double total) {
    //Database conneciton

    std::string sql;

    try {
        pqxx::connection C(
            "dbname = OCR user = yourserveruser password = yourserverpass \
        hostaddr = 127.0.0.1 port = 5432");
        if (C.is_open()) {
            std::cout << "Opened database successfully: " << C.dbname() << std::endl;
        }
        else {
            std::cout << "Can't open database" << std::endl;
            return 1;
        }

        /* Create SQL statement */
        std::string str1("INSERT INTO public.\"RECEIPTS\" (\"PURCH_TOTAL\") "  \
            "VALUES (); ");
        std::string str2(std::to_string(total));
        str1.insert(54, str2);
       
        
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
   //TODO: Needs to only look for the total line
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

int main() {
    OCR_read();
    total = parse();
    insert_DB(total);
    return 0;
}