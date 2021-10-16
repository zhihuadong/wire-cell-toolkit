#include "WireCellPytorch/TorchService.h"
#include "WireCellPytorch/Util.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"

WIRECELL_FACTORY(TorchService, 
                 WireCell::Pytorch::TorchService,
                 WireCell::ITensorForward,
                 WireCell::IConfigurable)

using namespace WireCell;

Pytorch::TorchService::TorchService()
    : Aux::Logger("TorchService", "torch")
{
}

Configuration Pytorch::TorchService::default_configuration() const
{
    Configuration cfg;

    // TorchScript model
    cfg["model"] = "model.ts";
    // one of: {cpu, gpu, gpucpu}.  Latter allows fail-over to cpu
    cfg["device"] = "gpu";      
    

    // if failed, wait this time and try again
    // cfg["wait_time"] = 500;  // ms

    // for debug
    // cfg["nloop"] = 10;

    return cfg;
}

void Pytorch::TorchService::configure(const WireCell::Configuration& cfg)
{
    auto dev = get<std::string>(cfg, "device", "gpucpu");

    auto model_path = cfg["model"].asString();
    if (model_path.empty()) {
        log->critical("no TorchScript model file provided");
        THROW(ValueError() << errmsg{"no TorchScript model file provided"});
    }

    // Load Torch Script Model.

    try {
        m_module = torch::jit::load(model_path);
    }
    catch (const c10::Error& e) {
        log->critical("error loading model: {}. {}", model_path, e.what());
        throw;                  // rethrow
    }
        
    if (dev == "gpu" or dev == "gpucpu") {
        m_gpu = true;
        try {
            m_module.to(at::kCUDA);
        }
        catch (const c10::Error& e) {
            log->warn("failed to load model to GPU, will use CPU: {}. {}", model_path, e.what());
            if (dev == "gpu") {
                throw;          // rethrow
            }
            m_gpu = false;
        }
    }
    if (dev == "cpu" or !m_gpu) {
        m_gpu = false;
        try {
            m_module.to(at::kCPU);
        }
        catch (const c10::Error& e) {
            log->critical("failed to load model to CPU: {}. {}", model_path, e.what());
            throw;                  // rethrow
        }
    }
        
    log->debug("loaded model {} to {}", model_path, m_gpu ? "GPU" : "CPU");
}

ITensorSet::pointer Pytorch::TorchService::forward(const ITensorSet::pointer& in) const
{
    // We may be called concurrently!
    // during runtime we must use the device that the model landed on
    // as per m_gpu.

    try {
        auto iival = Pytorch::from_itensor(in, m_gpu);
        auto oival = m_module.forward(iival);
        return Pytorch::to_itensor({oival});
    }
    catch (const std::runtime_error& err) {
        log->critical(err.what());
    }

    return nullptr;
}
