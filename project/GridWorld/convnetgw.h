#ifndef CONVNETGW_H
#define CONVNETGW_H
#include <torch/torch.h>

using namespace torch;
using namespace torch::nn ;

class ConvNetGW: Module
{
public:
    ConvNetGW();
    ConvNetGW(int nInputs, int nConv1, int nConv2, int nfc);
    Tensor forward(Tensor x);
    Tensor actorOutput(Tensor x);
    Tensor criticOutput(Tensor x);

private:
    int nInputs;
    int nConv1;
    int nConv2;
    int nfc;
    std::shared_ptr<Conv2d> conv1;
    std::shared_ptr<Conv2d> conv2;
    std::shared_ptr<LinearImpl> fc;
    std::shared_ptr<LinearImpl> actor;
    std::shared_ptr<LinearImpl> critic;
};

#endif // CONVNETGW_H
