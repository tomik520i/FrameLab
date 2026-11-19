#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <vector>
int wmain(int argc,wchar_t** argv){try{
 if(argc!=3)throw std::runtime_error("Usage: ocr_probe PNG_DIRECTORY OUTPUT_JSONL");
 winrt::init_apartment(winrt::apartment_type::multi_threaded);
 auto engine=winrt::Windows::Media::Ocr::OcrEngine::TryCreateFromLanguage(winrt::Windows::Globalization::Language(L"en-US"));
 if(!engine)throw std::runtime_error("English Windows OCR unavailable");
 std::ofstream out(argv[2]);std::vector<std::filesystem::path> paths;
 for(auto& entry:std::filesystem::directory_iterator(argv[1]))if(entry.path().extension()==L".png")paths.push_back(entry.path());
 std::sort(paths.begin(),paths.end());
 for(auto& path:paths){
  auto file=winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(std::filesystem::absolute(path).wstring()).get();
  auto stream=file.OpenAsync(winrt::Windows::Storage::FileAccessMode::Read).get();
  auto decoder=winrt::Windows::Graphics::Imaging::BitmapDecoder::CreateAsync(stream).get();
  auto bitmap=decoder.GetSoftwareBitmapAsync(winrt::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8,winrt::Windows::Graphics::Imaging::BitmapAlphaMode::Ignore).get();
  const auto start=std::chrono::steady_clock::now();auto result=engine.RecognizeAsync(bitmap).get();
  const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  out<<"{\"file\":"<<std::quoted(path.filename().string())<<",\"text\":"<<std::quoted(winrt::to_string(result.Text()))<<",\"ms\":"<<ms<<",\"lines\":[";
  bool first=true;for(auto line:result.Lines()){if(!first)out<<',';first=false;out<<std::quoted(winrt::to_string(line.Text()));}out<<"]}\n";out.flush();
  bitmap.Close();stream.Close();
 }
 return 0;
}catch(const winrt::hresult_error& e){std::cerr<<winrt::to_string(e.message());return 1;}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
