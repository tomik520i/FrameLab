#pragma once
#include "SubtitleModels.hpp"
#include "SubtitleTranslator.hpp"
#include "UiTheme.hpp"
#include "Localization.hpp"

inline bool subtitleComputePanel(SubtitleSettings& s,SubtitleModels& models,const SubtitleSnapshot& state){
 auto t=[](const char* en,const char* cs){return czech?cs:en;};
 ui::field(t("Translation compute backend","Výpočet překladu"));
 const char* ids[]{"auto","cuda","vulkan","cpu"};
 const char* labels[]{t("Automatic","Automaticky"),"CUDA / NVIDIA","Vulkan / AMD, Intel, NVIDIA","CPU / RAM"};
 int selected=0;for(int i=0;i<4;++i)if(s.computeBackend==ids[i])selected=i;
 if(ImGui::Combo("##translation-compute",&selected,labels,4)){s.computeBackend=ids[selected];s.computeDevice.clear();s.enabled=false;}
 const auto devices=models.devices();bool available=s.computeBackend=="auto"||s.computeBackend=="cpu";
 if(s.computeBackend=="cuda"||s.computeBackend=="vulkan"){
  const SubtitleComputeDevice* chosen=nullptr;bool any=false;
  for(const auto& d:devices)if(d.backend==s.computeBackend){any=true;if(d.id==s.computeDevice)chosen=&d;}
  available=s.computeDevice.empty()?any:chosen!=nullptr;
  ui::field(t("GPU for translation","GPU pro překlad"));
  const char* preview=s.computeDevice.empty()?t("Automatic within this backend","Automaticky pro tento backend"):chosen?chosen->name.c_str():t("Saved GPU is unavailable","Uložená GPU není dostupná");
  if(ImGui::BeginCombo("##translation-device",preview)){
   if(ImGui::Selectable(t("Automatic","Automaticky"),s.computeDevice.empty())){s.computeDevice.clear();s.enabled=false;}
   for(const auto& d:devices)if(d.backend==s.computeBackend){ImGui::PushID(d.id.c_str());const auto label=d.name+(d.integrated?t(" / shared RAM"," / sdílená RAM"):"");if(ImGui::Selectable(label.c_str(),s.computeDevice==d.id)){s.computeDevice=d.id;s.enabled=false;}ImGui::PopID();}
   ImGui::EndCombo();
  }
  if(!available)ImGui::TextWrapped("%s",models.hasDeviceList()?t("GPU not found. Refresh devices, update its driver, or choose CPU.","GPU nebyla nalezena. Obnov seznam, aktualizuj ovladač nebo zvol CPU."):t("Detecting translation GPUs…","Zjišťuji GPU pro překlad…"));
  if(chosen&&chosen->integrated)ImGui::TextWrapped("%s",t("Integrated GPU uses shared RAM. It can be much slower than a discrete GPU; measure subtitle delay.","Integrovaná GPU používá sdílenou RAM. Může být výrazně pomalejší než samostatná grafika; ověř prodlevu titulků."));
  if(chosen&&chosen->freeGiB>=0)ImGui::Text(t("Selected GPU: %.2f GiB free at last refresh","Vybraná GPU: při poslední obnově %.2f GiB volných"),chosen->freeGiB);
 }
 if(ImGui::SmallButton(t("Refresh translation GPUs","Obnovit GPU pro překlad")))models.refreshDevices();
 if(s.computeBackend=="auto")ImGui::TextWrapped("%s",t("Prefers a discrete GPU: CUDA on NVIDIA, Vulkan on other GPUs. Uses CPU when none is found. Independent of the image GPU.","Upřednostní samostatnou grafiku: CUDA na NVIDIA, Vulkan na ostatních GPU. Bez GPU použije CPU. Výběr je nezávislý na GPU pro obraz."));
 if(s.computeBackend!="cpu"){
  if(ImGui::Checkbox(t("Allow GPU + RAM offload (slower)","Povolit GPU + RAM (pomalejší)"),&s.allowCpuOffload))s.enabled=false;
  ImGui::TextWrapped("%s",s.allowCpuOffload?t("The runtime may place some model layers on CPU. Actual placement is shown after loading.","Runtime může část vrstev modelu umístit na CPU. Skutečné umístění se ukáže po načtení."):t("Full GPU loading is checked. If it does not fit, loading stops with an explanation.","Úplné načtení na GPU se ověřuje. Pokud se model nevejde, načítání skončí s vysvětlením."));
 }else ImGui::TextWrapped("%s",t("Runs the same GGUF model in system RAM. No CUDA or Vulkan required; expect a longer delay.","Stejný GGUF model běží v operační paměti. Nepotřebuje CUDA ani Vulkan; počítej s delší prodlevou."));
 if(!state.computeDescription.empty()&&state.activeModel==s.model)ImGui::TextWrapped("%s",state.computeDescription.c_str());
 return available;
}
