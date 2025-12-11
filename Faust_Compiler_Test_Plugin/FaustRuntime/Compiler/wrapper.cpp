#include "wrapper.h"
#include <faust/dsp/libfaust.h> 
#include <iostream>

CompilerWrapper::CompilerWrapper()
    : factory(nullptr)
    , mdsp(nullptr)
    , mapui(nullptr)
{}

CompilerWrapper::~CompilerWrapper()
{
    reset();
}

void CompilerWrapper::reset()
{
    if (mapui)
    {
        delete mapui;
        mapui = nullptr;
    }

    if (mdsp)
    {
        delete mdsp;
        mdsp = nullptr;
    }

    if (factory)
    {
        deleteDSPFactory(factory);
        factory = nullptr;
    }
}

bool CompilerWrapper::compileDSP(const std::string dspCode, const PluginConfiguration &cfg, std::string& errorMessage)
{

    if(factory){
        deleteDSPFactory(factory);
        factory = nullptr;
    }

    std::vector<const char*> args { 
        "-I", cfg.path.faust_dspdir.c_str(),
    };
    int argc = args.size();
    const char** argv = args.data();

    factory = createDSPFactoryFromString(
        cfg.name_app,
        dspCode,
        argc,
        argv,
        "",
        errorMessage,
        -1
    );

    if (!factory) 
    {
        std::cerr << "Faust compilation failed : " << errorMessage << std::endl;
        return false;
    }

    // update shaKey
    currentSHA = factory->getSHAKey();
    return true;
}

bool CompilerWrapper::createDSPandUI()
{
    mdsp = factory->createDSPInstance();
    if (!mdsp) {
        std::cerr << "Cannot create instance " << std::endl;
        return false;
    }

    mapui = new myMapUI();
    if (!mapui)
    {
        std::cerr<<" Cannot create mapui "<<std::endl;
        return false;
    }

    return true;
}

bool CompilerWrapper::configurePlugin(PluginConfiguration &cfg, ParameterList& parameters)
{
    mdsp->buildUserInterface(mapui);
    mapui->fillShortNames();
    setPluginConfiguration(cfg);
    setParameterList(parameters, cfg);
    return true; // TODO change return?
}

void CompilerWrapper::setupDSP(int sampleRate)
{
    mdsp->init(sampleRate);
}

FAUSTFLOAT CompilerWrapper::getParameter(const std::string& name)
{
    return mapui->getParamValue(name); 
}

void CompilerWrapper::setPluginConfiguration(PluginConfiguration &cfg)
{
    // Extract inputs/outputs
    cfg.num_inputs = mdsp->getNumInputs();
    cfg.num_outputs = mdsp->getNumOutputs();

    // Determine plugin type based on inputs
    if (cfg.num_inputs > 0) {
        cfg.plugin_type = "effect";
    } else {
        cfg.plugin_type = "source";
    }

    // Extract project name & override in case it is provided as a declaration in faust dsp file
    // i.e. declare name "Name"
    std::string pluginName = factory->getName(); // in case a declare name "name" is assigned by the user, otherwise cfg.app_name.
    cfg.plugin_name = PluginUtils::ensure_valid_plugin_name(pluginName);
    std::cout << "PLUGIN_NAME changed to " << cfg.plugin_name << std::endl;

    // @TODO: author name? description?
    cfg.author = "Unknown";
    cfg.description = "No description provided";
}

void CompilerWrapper::setParameterList(ParameterList& parameters, PluginConfiguration& cfg)
{
    
    const int numParams = mapui->getParamsCount(); 
    parameters.resize(numParams);

    for (const auto& control : mapui->controls)
    {
        myMapUI::itemInfo item = control.second;
        Parameter param;
        param.type = item.type;
        param.label = item.label;
        param.shortname = item.shortname;
        param.index = item.index;
        param.init = item.init;
        param.pmin = item.fmin;
        param.pmax = item.fmax;
        param.step = item.step;
        param.value.store(item.init);
        
        parameters[item.index] = std::move(param);
    }
}

void CompilerWrapper::callback(int bufferSize, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs)
{
    mdsp->compute(bufferSize, inputs, outputs);
}

bool CompilerWrapper::exportCPP(
    const std::string& faust_dspdir, 
    const std::string &filename, 
    const std::string& dspCode, 
    const std::string& filedir, 
    bool doublePrecision,
    std::string& errorMessage)
{

    std::vector<const char*> args { 
        "-I", faust_dspdir.c_str(),
        "-lang", "cpp" 
        "-o", filedir.c_str()
    };
    if (doublePrecision)
        args.push_back("-double");
    int argc = args.size();
    const char** argv = args.data();

    bool success = generateAuxFilesFromString(
        filename,
        dspCode,
        argc,
        argv,
        errorMessage
    );

    if (!success) 
    {
        std::cerr << "Faust C++ export failed: " << errorMessage << std::endl;
        return false;
    }
    return true;
}
