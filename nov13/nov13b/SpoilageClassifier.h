#pragma once
#include <cstdarg>
namespace Eloquent {
    namespace ML {
        namespace Port {
            class SpoilageClassifier {
                public:
                    /**
                    * Predict class for features vector
                    */
                    int predict(float *x) {
                        uint8_t votes[3] = { 0 };
                        // tree #1
                        if (x[0] <= 33.614999771118164) {
                            if (x[1] <= 71.17500305175781) {
                                votes[1] += 1;
                            }

                            else {
                                if (x[1] <= 71.43500137329102) {
                                    if (x[1] <= 71.36999893188477) {
                                        if (x[0] <= 32.80000114440918) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }

                                    else {
                                        if (x[0] <= 32.78499984741211) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }
                                }

                                else {
                                    if (x[0] <= 32.80500030517578) {
                                        votes[0] += 1;
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }
                            }
                        }

                        else {
                            votes[2] += 1;
                        }

                        // tree #2
                        if (x[2] <= 9.555999755859375) {
                            if (x[0] <= 33.6200008392334) {
                                if (x[0] <= 32.790000915527344) {
                                    votes[0] += 1;
                                }

                                else {
                                    if (x[1] <= 71.30500030517578) {
                                        if (x[0] <= 33.295000076293945) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }
                            }

                            else {
                                votes[2] += 1;
                            }
                        }

                        else {
                            votes[2] += 1;
                        }

                        // tree #3
                        if (x[1] <= 70.03000259399414) {
                            votes[2] += 1;
                        }

                        else {
                            if (x[1] <= 71.43000030517578) {
                                if (x[1] <= 71.24499893188477) {
                                    if (x[0] <= 32.295000076293945) {
                                        votes[0] += 1;
                                    }

                                    else {
                                        if (x[1] <= 71.21500015258789) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[0] += 1;
                                        }
                                    }
                                }

                                else {
                                    if (x[1] <= 71.30500030517578) {
                                        if (x[1] <= 71.27000045776367) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }

                                    else {
                                        if (x[0] <= 32.790000915527344) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }
                                }
                            }

                            else {
                                votes[0] += 1;
                            }
                        }

                        // tree #4
                        if (x[2] <= 13.143500328063965) {
                            if (x[1] <= 70.03000259399414) {
                                votes[2] += 1;
                            }

                            else {
                                if (x[0] <= 32.790000915527344) {
                                    votes[0] += 1;
                                }

                                else {
                                    if (x[0] <= 33.295000076293945) {
                                        votes[1] += 1;
                                    }

                                    else {
                                        if (x[1] <= 71.33499908447266) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }
                                }
                            }
                        }

                        else {
                            votes[2] += 1;
                        }

                        // tree #5
                        if (x[2] <= 9.555999755859375) {
                            if (x[1] <= 70.02000045776367) {
                                votes[2] += 1;
                            }

                            else {
                                if (x[0] <= 32.790000915527344) {
                                    votes[0] += 1;
                                }

                                else {
                                    if (x[0] <= 33.295000076293945) {
                                        votes[1] += 1;
                                    }

                                    else {
                                        if (x[0] <= 33.30500030517578) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }
                                }
                            }
                        }

                        else {
                            votes[2] += 1;
                        }

                        // tree #6
                        if (x[1] <= 70.01499938964844) {
                            votes[2] += 1;
                        }

                        else {
                            if (x[1] <= 71.19499969482422) {
                                if (x[1] <= 71.15500259399414) {
                                    if (x[0] <= 33.31500053405762) {
                                        votes[1] += 1;
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }

                                else {
                                    votes[1] += 1;
                                }
                            }

                            else {
                                if (x[1] <= 71.28499984741211) {
                                    if (x[0] <= 32.80500030517578) {
                                        votes[0] += 1;
                                    }

                                    else {
                                        if (x[0] <= 33.30500030517578) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[0] += 1;
                                        }
                                    }
                                }

                                else {
                                    if (x[0] <= 32.790000915527344) {
                                        votes[0] += 1;
                                    }

                                    else {
                                        if (x[0] <= 33.30500030517578) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }
                                }
                            }
                        }

                        // tree #7
                        if (x[1] <= 70.03000259399414) {
                            votes[2] += 1;
                        }

                        else {
                            if (x[1] <= 71.44499969482422) {
                                if (x[0] <= 32.790000915527344) {
                                    votes[0] += 1;
                                }

                                else {
                                    if (x[1] <= 71.31999969482422) {
                                        if (x[0] <= 33.295000076293945) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[0] += 1;
                                        }
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }
                            }

                            else {
                                if (x[0] <= 32.78499984741211) {
                                    votes[0] += 1;
                                }

                                else {
                                    votes[1] += 1;
                                }
                            }
                        }

                        // tree #8
                        if (x[0] <= 33.6200008392334) {
                            if (x[1] <= 71.22999954223633) {
                                if (x[0] <= 32.80000114440918) {
                                    votes[0] += 1;
                                }

                                else {
                                    if (x[1] <= 71.17500305175781) {
                                        if (x[1] <= 71.15500259399414) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[0] += 1;
                                        }
                                    }

                                    else {
                                        if (x[0] <= 33.295000076293945) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }
                                }
                            }

                            else {
                                if (x[1] <= 71.33499908447266) {
                                    if (x[0] <= 32.80000114440918) {
                                        votes[0] += 1;
                                    }

                                    else {
                                        if (x[1] <= 71.27000045776367) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[0] += 1;
                                        }
                                    }
                                }

                                else {
                                    if (x[1] <= 71.36999893188477) {
                                        if (x[0] <= 32.80000114440918) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }

                                    else {
                                        if (x[1] <= 71.41500091552734) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }
                                }
                            }
                        }

                        else {
                            votes[2] += 1;
                        }

                        // tree #9
                        if (x[2] <= 3.8125) {
                            if (x[1] <= 70.01499938964844) {
                                votes[2] += 1;
                            }

                            else {
                                if (x[0] <= 32.790000915527344) {
                                    votes[0] += 1;
                                }

                                else {
                                    if (x[0] <= 33.295000076293945) {
                                        votes[1] += 1;
                                    }

                                    else {
                                        if (x[1] <= 71.28499984741211) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }
                                }
                            }
                        }

                        else {
                            votes[2] += 1;
                        }

                        // tree #10
                        if (x[0] <= 33.6200008392334) {
                            if (x[1] <= 71.28499984741211) {
                                if (x[0] <= 32.795000076293945) {
                                    votes[0] += 1;
                                }

                                else {
                                    if (x[0] <= 33.295000076293945) {
                                        votes[1] += 1;
                                    }

                                    else {
                                        if (x[1] <= 71.22999954223633) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[0] += 1;
                                        }
                                    }
                                }
                            }

                            else {
                                if (x[1] <= 71.43500137329102) {
                                    if (x[1] <= 71.30500030517578) {
                                        votes[1] += 1;
                                    }

                                    else {
                                        if (x[1] <= 71.33499908447266) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }
                                }

                                else {
                                    if (x[1] <= 71.46500015258789) {
                                        if (x[0] <= 32.80500030517578) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }

                                    else {
                                        votes[0] += 1;
                                    }
                                }
                            }
                        }

                        else {
                            votes[2] += 1;
                        }

                        // tree #11
                        if (x[0] <= 33.6200008392334) {
                            if (x[0] <= 32.790000915527344) {
                                votes[0] += 1;
                            }

                            else {
                                if (x[1] <= 71.24499893188477) {
                                    if (x[0] <= 33.32000160217285) {
                                        if (x[0] <= 33.30500030517578) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[0] += 1;
                                        }
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }

                                else {
                                    votes[1] += 1;
                                }
                            }
                        }

                        else {
                            votes[2] += 1;
                        }

                        // tree #12
                        if (x[2] <= 3.8125) {
                            if (x[1] <= 70.01499938964844) {
                                votes[2] += 1;
                            }

                            else {
                                if (x[0] <= 32.790000915527344) {
                                    votes[0] += 1;
                                }

                                else {
                                    if (x[1] <= 71.33499908447266) {
                                        if (x[0] <= 33.30500030517578) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }
                            }
                        }

                        else {
                            votes[2] += 1;
                        }

                        // tree #13
                        if (x[2] <= 4.868500232696533) {
                            if (x[0] <= 32.790000915527344) {
                                votes[0] += 1;
                            }

                            else {
                                if (x[0] <= 33.61000061035156) {
                                    if (x[1] <= 71.30500030517578) {
                                        if (x[1] <= 71.21500015258789) {
                                            votes[1] += 1;
                                        }

                                        else {
                                            votes[0] += 1;
                                        }
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }

                                else {
                                    votes[2] += 1;
                                }
                            }
                        }

                        else {
                            votes[2] += 1;
                        }

                        // tree #14
                        if (x[2] <= 3.8125) {
                            if (x[0] <= 33.6200008392334) {
                                if (x[1] <= 71.33499908447266) {
                                    if (x[0] <= 32.80000114440918) {
                                        votes[0] += 1;
                                    }

                                    else {
                                        if (x[1] <= 71.17500305175781) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[0] += 1;
                                        }
                                    }
                                }

                                else {
                                    if (x[1] <= 71.43500137329102) {
                                        if (x[0] <= 32.795000076293945) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }

                                    else {
                                        if (x[1] <= 71.46999740600586) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[0] += 1;
                                        }
                                    }
                                }
                            }

                            else {
                                votes[2] += 1;
                            }
                        }

                        else {
                            votes[2] += 1;
                        }

                        // tree #15
                        if (x[1] <= 70.02000045776367) {
                            votes[2] += 1;
                        }

                        else {
                            if (x[1] <= 71.21500015258789) {
                                if (x[0] <= 32.80000114440918) {
                                    votes[0] += 1;
                                }

                                else {
                                    if (x[1] <= 71.15500259399414) {
                                        votes[0] += 1;
                                    }

                                    else {
                                        votes[1] += 1;
                                    }
                                }
                            }

                            else {
                                if (x[0] <= 32.790000915527344) {
                                    votes[0] += 1;
                                }

                                else {
                                    if (x[0] <= 33.295000076293945) {
                                        votes[1] += 1;
                                    }

                                    else {
                                        if (x[1] <= 71.31999969482422) {
                                            votes[0] += 1;
                                        }

                                        else {
                                            votes[1] += 1;
                                        }
                                    }
                                }
                            }
                        }

                        // return argmax of votes
                        uint8_t classIdx = 0;
                        float maxVotes = votes[0];

                        for (uint8_t i = 1; i < 3; i++) {
                            if (votes[i] > maxVotes) {
                                classIdx = i;
                                maxVotes = votes[i];
                            }
                        }

                        return classIdx;
                    }

                protected:
                };
            }
        }
    }