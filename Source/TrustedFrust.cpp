#include "TrustedFrust.h"
#include <CompilerApi.h>
#include <frust_plugin_host/FrustPluginHost.h>
#include <ResearchProofSources.h>
#include <memory>

// Existing FRust runtime export, emitted at source locations by the compiler.
extern "C" void frust_dbg_tick(int line, int column);

TrustedFrustProof runTrustedFrustProof()
{
    TrustedFrustProof result;
    result.log = "FRust source/header/library revision: " RESEARCH_FRUST_REVISION "\n";
    frust::CompileRequest request;
    request.sources.push_back({"answer.frust", std::string(ResearchProofSources::answer_frust, ResearchProofSources::answer_frustSize)});
    const auto compiled = frust::Compile(request);
    for (const auto& d : compiled.diagnostics) result.log += juce::String(frust::FormatDiagnostic(d)) + "\n";
    if (compiled.ok)
    {
        frust_plugin_host_set_application_identity("djehuti-research");
        // Referencing the real export also retains RuntimeBasic.cpp from the
        // static runtime archive. No replacement debugger/runtime is created.
        frust_plugin_register_host_function("frust_dbg_tick", reinterpret_cast<void*>(&frust_dbg_tick));
        // The existing host API owns executable memory. Entry points are used
        // only inside the handle's scope, never cached across unload.
        using Handle = std::unique_ptr<FrustPluginHandleImpl, decltype(&frust_plugin_unload)>;
        constexpr auto manifest = R"({"name":"research-foundation-proof","version":"1.0.0","intendedApplications":["djehuti-research"],"entryPoints":["answer"]})";
        for (int i = 0; i < 3; ++i)
        {
            Handle handle(frust_plugin_load_object("research-proof", manifest, compiled.object.data(), compiled.object.size()), &frust_plugin_unload);
            if (!handle) { result.log += "FAIL load: " + juce::String(frust_plugin_last_error()) + "\n"; break; }
            using Answer = std::int64_t (*)();
            const auto answer = reinterpret_cast<Answer>(frust_plugin_get_fn(handle.get(), "answer"));
            if (answer == nullptr) { result.log += "FAIL: entry point 'answer' could not be resolved. " + juce::String(frust_plugin_last_error()) + "\n"; break; }
            result.answer = answer();
            if (result.answer != 42) break;
            ++result.cycles;
        }
        result.answerPassed = result.cycles > 0 && result.answer == 42;
        result.lifecyclePassed = result.cycles == 3;
    }
    result.log += juce::String(result.answerPassed ? "PASS" : "FAIL") + ": answer() -> " + juce::String(result.answer) + " (expected 42)\n";
    result.log += juce::String(result.lifecyclePassed ? "PASS" : "FAIL") + ": " + juce::String(result.cycles) + "/3 load / invoke / unload cycles\n";
    request.sources = {{"invalid.frust", std::string(ResearchProofSources::invalid_frust, ResearchProofSources::invalid_frustSize)}};
    const auto invalid = frust::Compile(request);
    for (const auto& diagnostic : invalid.diagnostics) result.diagnostic += juce::String(frust::FormatDiagnostic(diagnostic)) + "\n";
    result.diagnosticPassed = !invalid.ok && invalid.object.empty() && result.diagnostic.contains("invalid.frust:") && result.diagnostic.isNotEmpty();
    result.log += juce::String(result.diagnosticPassed ? "PASS" : "FAIL") + ": malformed input rejected\n" + result.diagnostic;
    result.log += result.passed() ? "Trusted FRust self-test PASSED. Compiled handles unloaded.\n" : "Trusted FRust self-test FAILED.\n";
    return result;
}
