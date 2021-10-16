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
    , m_dev(torch::kCPU, 0)
    , m_sem(0)
{
}

Configuration Pytorch::TorchService::default_configuration() const
{
    Configuration cfg;

    // TorchScript model
    cfg["model"] = "model.ts";
    // one of: {cpu, gpu, gpucpu}.  Latter allows fail-over to cpu
    // when there is a failure to load the model.
    cfg["device"] = "gpucpu";      
    
    // The number of concurrent thread access to forward().  Value is
    // how many additional threads.  Eg, 0 means no concurrency, only
    // one thread total may run.  Value of 1 means one additional
    // thread may run.
    cfg["concurrency"] = 0;

    return cfg;
}

void Pytorch::TorchService::configure(const WireCell::Configuration& cfg)
{
    auto dev = get<std::string>(cfg, "device", "gpucpu");
    auto count = get<int>(cfg, "concurrency", 0);
    m_sem.set_count(count);

    auto model_path = cfg["model"].asString();
    if (model_path.empty()) {
        log->critical("no TorchScript model file provided");
        THROW(ValueError() << errmsg{"no TorchScript model file provided"});
    }

    // Load Torch Script Model.
    if (dev == "gpucpu") {
        try {
            m_module = torch::jit::load(model_path, torch::kCUDA);
        }
        catch (const c10::Error& e) {
            log->warn("failed to load model: {} to GPU will try CPU: {}",
                      model_path, e.what());
            dev = "cpu";
        }
    }        

    // from now, we either succeed or we throw

    // Use almost 1/2 the memory and 3/4 the time.
    // but, fixme: check with Haiwng that this is okay.
    torch::NoGradGuard no_grad;

    if (dev == "cpu") {
        m_dev = torch::Device(torch::kCPU, 0);
    }
    else {
        m_dev = torch::Device(torch::kCUDA, 0);
    }

    try {
        m_module = torch::jit::load(model_path, m_dev);
    }
    catch (const c10::Error& e) {
        log->critical("error loading model: {} to {}: {}",
                      model_path, dev, e.what());
        throw;                  // rethrow
    }

    const bool is_gpu = ! (m_dev == torch::kCPU);
    log->debug("loaded model {} to {}",
               model_path, is_gpu ? "GPU" : "CPU");
}

#include <c10/cuda/CUDACachingAllocator.h>

ITensorSet::pointer Pytorch::TorchService::forward(const ITensorSet::pointer& in) const
{
    m_sem.acquire();

    torch::NoGradGuard no_grad;

    const bool is_gpu = ! (m_dev == torch::kCPU);
    std::vector<torch::IValue> iival = Pytorch::from_itensor(in, is_gpu);

    torch::IValue oival;
    try {
        oival = m_module.forward(iival);
    }
    catch (const std::runtime_error& err) {
        log->error("error running model on {}: {}",
                   is_gpu ? "GPU" : "CPU", err.what());
        m_sem.release();
        return nullptr;
    }

    ITensorSet::pointer ret = Pytorch::to_itensor({oival});

    // maybe needs a mutex?
    c10::cuda::CUDACachingAllocator::emptyCache();

    m_sem.release();
    return ret;
}
