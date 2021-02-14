#include "module_noaa_avhrr.h"
#include <fstream>
#include "avhrr_reader.h"
#include <ccsds/demuxer.h>
#include <ccsds/vcdu.h>
#include "logger.h"
#include <filesystem>

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

NOAAAVHRRDecoderModule::NOAAAVHRRDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
{
}

void NOAAAVHRRDecoderModule::process()
{
    size_t filesize = getFilesize(d_input_file);
    std::ifstream data_in(d_input_file, std::ios::binary);

    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/AVHRR";

    logger->info("Using input frames " + d_input_file);
    logger->info("Decoding to " + directory);

    time_t lastTime = 0;

    noaa::AVHRRReader reader;

    uint16_t buffer[11090];

    logger->info("Demultiplexing and deframing...");

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)buffer, 11090 * 2);

        reader.work(buffer);

        if (time(NULL) % 10 == 0 && lastTime != time(NULL))
        {
            lastTime = time(NULL);
            logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%");
        }
    }

    data_in.close();

    logger->info("AVHRR Lines            : " + std::to_string(reader.lines));

    logger->info("Writing images.... (Can take a while)");

    if (!std::filesystem::exists(directory))
        std::filesystem::create_directory(directory);

    cimg_library::CImg<unsigned short> image1 = reader.getChannel(0);
    cimg_library::CImg<unsigned short> image2 = reader.getChannel(1);
    cimg_library::CImg<unsigned short> image3 = reader.getChannel(2);
    cimg_library::CImg<unsigned short> image4 = reader.getChannel(3);
    cimg_library::CImg<unsigned short> image5 = reader.getChannel(4);

    logger->info("Channel 1...");
    image1.save_png(std::string(directory + "/AVHRR-1.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-1.png");

    logger->info("Channel 2...");
    image2.save_png(std::string(directory + "/AVHRR-2.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-2.png");

    logger->info("Channel 3...");
    image3.save_png(std::string(directory + "/AVHRR-3.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-3.png");

    logger->info("Channel 4...");
    image4.save_png(std::string(directory + "/AVHRR-4.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-4.png");

    logger->info("Channel 5...");
    image5.save_png(std::string(directory + "/AVHRR-5.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-5.png");

    logger->info("221 Composite...");
    cimg_library::CImg<unsigned short> image221(2048, reader.lines, 1, 3);
    {
        image221.draw_image(0, 0, 0, 0, image2);
        image221.draw_image(0, 0, 0, 1, image2);
        image221.draw_image(0, 0, 0, 2, image1);
    }
    image221.save_png(std::string(directory + "/AVHRR-RGB-221.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-RGB-221.png");
    image221.equalize(1000);
    image221.normalize(0, std::numeric_limits<unsigned char>::max());
    image221.save_png(std::string(directory + "/AVHRR-RGB-221-EQU.png").c_str());
    d_output_files.push_back(directory + "/AVHRR-RGB-221-EQU.png");
}

std::string NOAAAVHRRDecoderModule::getID()
{
    return "noaa_avhrr";
}

std::vector<std::string> NOAAAVHRRDecoderModule::getParameters()
{
    return {};
}

std::shared_ptr<ProcessingModule> NOAAAVHRRDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
{
    return std::make_shared<NOAAAVHRRDecoderModule>(input_file, output_file_hint, parameters);
}