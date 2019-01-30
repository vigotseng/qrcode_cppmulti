//
//  file:  %{Cpp:License:FileName}
//
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
//
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fstream>
#include <memory>
#include <list>
#include <string>

#include <opencv2/opencv.hpp>
#include <qrencode.h>
#include <pthread.h>

using namespace std;
using namespace cv;

const char base_url[] = "http://csgyn.com/t/?qr=";
const int  base_url_length = strlen(base_url);
Mat logo_img;

void *make_qr_label(void *param)
{
    const char* line = reinterpret_cast<const char*>(param);
    int line_length = strlen(line);

    unique_ptr<char> qr_content = unique_ptr<char>(new char[base_url_length + line_length]);

    strncpy(qr_content.get(), base_url, base_url_length);
    strncpy(qr_content.get() + base_url_length, line, line_length);

    unique_ptr<char> file_name = unique_ptr<char>(new char[line_length + 5]);
    strncpy(file_name.get(), line, line_length);
    strncpy(file_name.get() + line_length, ".png", 5);

    QRcode *pQRcode = QRcode_encodeString(qr_content.get(), 1, QR_ECLEVEL_Q, QR_MODE_8, 1);

    int scale = 25;
    int spacing = 25;
    Mat qr_img(2 * spacing + pQRcode->width * scale + 75, 2 * spacing + pQRcode->width * scale, CV_8UC3);

    // fill image with white
    memset(qr_img.data, 0xff, qr_img.cols * qr_img.rows * qr_img.channels());

    // draw qr code
    uchar *pSrc = pQRcode->data;
    for (int col = 0; col < pQRcode->width; ++col) {
        for (int row = 0; row < pQRcode->width; ++row) {
            if (*pSrc & 1) {
                for (int i = 0; i < scale; ++i) {
                    for (int j = 0; j < scale; ++j) {
                        qr_img.at<Vec3b>(spacing + col * scale + j, spacing + row * scale + i)[0] = 0;
                        qr_img.at<Vec3b>(spacing + col * scale + j, spacing + row * scale + i)[1] = 0;
                        qr_img.at<Vec3b>(spacing + col * scale + j, spacing + row * scale + i)[2] = 0;
                    }
                }
            } else {
                for (int i = 0; i < scale; ++i) {
                    for (int j = 0; j < scale; ++j) {
                        qr_img.at<Vec3b>(spacing + col * scale + j, spacing + row * scale + i)[0] = 0xff;
                        qr_img.at<Vec3b>(spacing + col * scale + j, spacing + row * scale + i)[1] = 0xff;
                        qr_img.at<Vec3b>(spacing + col * scale + j, spacing + row * scale + i)[2] = 0xff;
                    }
                }
            }
            pSrc++;
        }
    }
    QRcode_free(pQRcode);

    // draw logo in the center
    int place_posx = (qr_img.cols - logo_img.cols) / 2;
    int place_posy = place_posx;
    logo_img.copyTo(qr_img.rowRange(place_posx, place_posx + logo_img.cols).colRange(place_posy, place_posy + logo_img.rows));

    // draw text
    Size textSize = getTextSize(line, FONT_HERSHEY_SCRIPT_SIMPLEX, 2, 5, nullptr);
    int text_posx = (qr_img.cols - textSize.width) / 2;
    int text_posy = (qr_img.rows - textSize.height / 2) ;
    putText(qr_img, line, Point(text_posx, text_posy), FONT_HERSHEY_SCRIPT_SIMPLEX, 2, Scalar(0, 0, 0, 0), 5);

    imwrite(file_name.get(), qr_img);

    return nullptr;
}

int start_multithread_task(const list<string> &file_content, int thread_cnt)
{
    pthread_t thread_pool[thread_cnt];

    int thread_idx = 0;

    int total = file_content.size();
    int idx = 0;
    int last_percentage = 0;

    for (string line : file_content) {
        pthread_create(&thread_pool[thread_idx++], nullptr, make_qr_label, (void*)line.c_str());

        if (thread_idx == thread_cnt) {
            for (int i = 0; i < thread_cnt; ++i) {
                pthread_join(thread_pool[i], nullptr);
            }
            thread_idx = 0;
            idx += thread_cnt;
        }

        int percentage = ((idx * 100) /total);

        if (percentage != last_percentage) {
            fprintf(stdout, "%3d%% ", percentage);
            for (int i = 0; i < percentage; ++i) {
                fprintf(stdout, "#");
            }
            fprintf(stdout, "\n");
            last_percentage = percentage;
        }
    }

    for (int i = 0; i < thread_cnt; ++i) {
        pthread_join(thread_pool[i], nullptr);
    }

    fprintf(stdout, "Finished %d QRCode.\n", total);
    return 0;
}

int load_input_file(const char *file_path, int thread_cnt)
{
    ifstream inputfile(file_path);
    if (!inputfile.is_open()) {
        perror("Can't open input file !");
        return -1;
    }

    list<string> file_content;
    while (!inputfile.eof()) {
        char line[1024];
        inputfile.getline(line, 1024);

        if (strlen(line)) {
            file_content.push_back(line);
        }
    }
    inputfile.close();

    return start_multithread_task(file_content, thread_cnt);
}

void usage(void)
{
    fprintf(stdout, "Usage: ./make_qr_label input.txt [thread_cnt]\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage();
        exit(-1);
    }

    int thread_cnt = (argc == 3) ? atoi(argv[2]) : 1;

    logo_img = imread("logo.png");
    if (!logo_img.data) {
        perror("Can't open logo.png!");
        exit(-1);
    }
    resize(logo_img, logo_img, Size(200, 200));

    return load_input_file(argv[1], thread_cnt);
}
