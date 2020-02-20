#include "WireCellPytorch/TorchScript.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"

WIRECELL_FACTORY(TorchScript, WireCell::Pytorch::TorchScript,
                 WireCell::Pytorch::ITorchScript, WireCell::IConfigurable)

using namespace WireCell;

Pytorch::TorchScript::TorchScript() : m_ident(0), l(Log::logger("pytorch")) {}

Configuration Pytorch::TorchScript::default_configuration() const {
  Configuration cfg;

  // TorchScript model
  cfg["model"] = "model.ts";
  cfg["gpu"] = true;

  // if failed, wait this time and try again
  cfg["wait_time"] = 500; // ms

  return cfg;
}

void Pytorch::TorchScript::configure(const WireCell::Configuration &cfg) {

  m_cfg = cfg;

  auto model_path = m_cfg["model"].asString();
  if (model_path.empty()) {
    THROW(ValueError() << errmsg{"Must provide output model to TorchScript"});
  }

  // load Torch Script Model
  try {
    // Deserialize the ScriptModule from a file using torch::jit::load().
    m_module = torch::jit::load(m_cfg["model"].asString());
    if (m_cfg["gpu"].asBool())
      m_module.to(at::kCUDA);
    else
      m_module.to(at::kCPU);
    l->info("Model: {} loaded", m_cfg["model"].asString());
  } catch (const c10::Error &e) {
    l->critical("error loading model: {}", m_cfg["model"].asString());
    exit(0);
  }

  m_timers["total_wait_time"] = 0;
}

torch::IValue
Pytorch::TorchScript::forward(const std::vector<torch::IValue> &inputs) {
  torch::IValue ret;
  int wait_time = m_cfg["wait_time"].asInt();

  bool success = false;
  while (!success) {
    try {
      ret = m_module.forward(inputs);
      success = true;
    } catch (...) {
      std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
      m_timers["total_wait_time"] += wait_time;
    }
  }

  l->info("total_wait_time: {} sec", m_timers["total_wait_time"] / 1000.);

  return ret;
}