#include "module_metop_mhs.h"
#include <fstream>
#include "mhs_reader.h"
#include <ccsds/demuxer.h>
#include <ccsds/vcdu.h>
#include "logger.h"
#include <filesystem>

#define BUFFER_SIZE 8192

// Return filesize
size_t getFilesize(std::string filepath);

MetOpMHSDecoderModule::MetOpMHSDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
{
}

void MetOpMHSDecoderModule::process()
{
    size_t filesize = getFilesize(d_input_file);
    std::ifstream data_in(d_input_file, std::ios::binary);

    std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/MHS";

    logger->info("Using input frames " + d_input_file);
    logger->info("Decoding to " + directory);

    time_t lastTime = 0;

    libccsds::Demuxer ccsdsDemuxer(882, true);
    MHSReader mhsreader;
    uint64_t mhs_cadu = 0, ccsds = 0, mhs_ccsds = 0;
    libccsds::CADU cadu;

    logger->info("Demultiplexing and deframing...");

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)&cadu, 1024);

        // Parse this transport frame
        libccsds::VCDU vcdu = libccsds::parseVCDU(cadu);

        // Right channel? (VCID 12 is MHS)
        if (vcdu.vcid == 12)
        {
            mhs_cadu++;

            // Demux
            std::vector<libccsds::CCSDSPacket> ccsdsFrames = ccsdsDemuxer.work(cadu);

            // Count frames
            ccsds += ccsdsFrames.size();

            // Push into processor (filtering APID 64)
            for (libccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                if (pkt.header.apid == 34)
                {
                    mhsreader.work(pkt);
                    mhs_ccsds++;
                }
            }
        }

        if (time(NULL) % 10 == 0 && lastTime != time(NULL))
        {
            lastTime = time(NULL);
            logger->info("Progress " + std::to_string(round(((float)data_in.tellg() / (float)filesize) * 1000.0f) / 10.0f) + "%");
        }
    }

    data_in.close();

    logger->info("VCID 12 (MHS) Frames : " + std::to_string(mhs_cadu));
    logger->info("CCSDS Frames         : " + std::to_string(ccsds));
    logger->info("MHS Frames           : " + std::to_string(mhs_ccsds));

    logger->info("Writing images.... (Can take a while)");

    if (!std::filesystem::exists(directory))
        std::filesystem::create_directory(directory);

    for (int i = 0; i < 5; i++)
    {
        logger->info("Channel " + std::to_string(i + 1) + "...");
        mhsreader.getChannel(i).save_png(std::string(directory + "/MHS-" + std::to_string(i + 1) + ".png").c_str());
        d_output_files.push_back(directory + "/MHS-" + std::to_string(i + 1) + ".png");
    }

    // Output a few nice composites as well
    logger->info("Global Composite...");
    cimg_library::CImg<unsigned short> imageAll(90 * 3, mhsreader.getChannel(0).height() * 2, 1, 1);
    {
        int height = mhsreader.getChannel(0).height();

        // Row 1
        imageAll.draw_image(90 * 0, 0, 0, 0, mhsreader.getChannel(0));
        imageAll.draw_image(90 * 1, 0, 0, 0, mhsreader.getChannel(1));
        imageAll.draw_image(90 * 2, 0, 0, 0, mhsreader.getChannel(2));

        // Row 2
        imageAll.draw_image(90 * 0, height, 0, 0, mhsreader.getChannel(3));
        imageAll.draw_image(90 * 1, height, 0, 0, mhsreader.getChannel(4));
    }
    imageAll.save_png(std::string(directory + "/MHS-ALL.png").c_str());
    d_output_files.push_back(directory + "/MHS-ALL.png");
}

std::string MetOpMHSDecoderModule::getID()
{
    return "metop_mhs";
}

std::vector<std::string> MetOpMHSDecoderModule::getParameters()
{
    return {};
}

std::shared_ptr<ProcessingModule> MetOpMHSDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
{
    return std::make_shared<MetOpMHSDecoderModule>(input_file, output_file_hint, parameters);
}