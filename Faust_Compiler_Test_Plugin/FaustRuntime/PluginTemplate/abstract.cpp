#include "abstract.h"

AbstractPlugin::AbstractPlugin(PluginConfiguration& config, ParameterList& params, CompilerWrapper& compiler)
    : cfg(config)
    , parameters(params)
    , faustCompiler(compiler)
{
}

AbstractPlugin::~AbstractPlugin(){
    reset();
}

void AbstractPlugin::setup()
{
    faust_outputs.resize(cfg.num_outputs);
}

void AbstractPlugin::reset()
{
    faust_outputs.clear();

}

void AbstractPlugin::setParameters()
{
    for (auto& p : parameters){

        if (p.type!="bargraph")
        {
            faustCompiler.setParameter(p.shortname,p.value.load());
        }
        else
        {
            p.value.store(faustCompiler.getParameter(p.shortname));
        }

    }

}