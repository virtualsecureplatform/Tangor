#include <wrappedcodelets.hpp>

namespace Tangor{

static struct starpu_perfmodel homgate_perf_model =
{
    .type = STARPU_HISTORY_BASED,
    .symbol = "HomGate_perf_model"
};

struct starpu_codelet clHomNAND =
{
    .where = STARPU_CPU,
    .cpu_funcs = {  HomGateWrap<P, -1, -1, P::μ>
                },
    .cpu_funcs_name = { "HomNAND" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomNOR =
{
    .where = STARPU_CPU,
    .cpu_funcs = {  HomGateWrap<P, -1, -1, -P::μ>
    },
    .cpu_funcs_name = { "HomNOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomXNOR =
{
    .where = STARPU_CPU,
    .cpu_funcs = {  HomGateWrap<P, -2, -2, -2*P::μ>
     },
    .cpu_funcs_name = { "HomXNOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomAND =
{
    .where = STARPU_CPU,
    .cpu_funcs = {  HomGateWrap<P, 1, 1, -P::μ>
    },
    .cpu_funcs_name = { "HomAND" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomOR =
{
    .where = STARPU_CPU,
    .cpu_funcs = {  HomGateWrap<P, 1, 1, P::μ>
    },
    .cpu_funcs_name = { "HomOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomXOR =
{
    .where = STARPU_CPU,
    .cpu_funcs = {  HomGateWrap<P, 2, 2, 2*P::μ>
     },
    .cpu_funcs_name = { "HomXOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomANDNY =
{
    .where = STARPU_CPU,
    .cpu_funcs = {  HomGateWrap<P, -1, 1, -P::μ>
    },
    .cpu_funcs_name = { "HomANDNY" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomANDYN =
{
    .where = STARPU_CPU,
    .cpu_funcs = {  HomGateWrap<P, 1, -1, -P::μ>
                },
    .cpu_funcs_name = { "HomANDYN" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomORNY =
{
    .where = STARPU_CPU,
    .cpu_funcs = {  HomGateWrap<P, -1, 1, P::μ>
                },
    .cpu_funcs_name = { "HomORNY" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomORYN =
{
    .where = STARPU_CPU,
    .cpu_funcs = {  HomGateWrap<P, 1, -1, P::μ>},
    .cpu_funcs_name = { "HomORYN" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

}