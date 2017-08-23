    
    #include "track2steer.h"

    track2steer_obj * track2steer_construct_zero(const unsigned int nSeps, const unsigned int nChannels, const unsigned int halfFrameSize, const float c, const unsigned int fS, const float * mics) {

        track2steer_obj * obj;
        unsigned int iBin;

        obj = (track2steer_obj *) malloc(sizeof(track2steer_obj));

        obj->nSeps = nSeps;
        obj->nChannels = nChannels;
        obj->halfFrameSize = halfFrameSize;
        obj->c = c;
        obj->fS = fS;

        obj->mics = (float *) malloc(sizeof(float) * nChannels * 3);
        memcpy(obj->mics, mics, sizeof(float) * nChannels * 3);        

        obj->factor = (float *) malloc(sizeof(float) * halfFrameSize);

        for (iBin = 0; iBin < halfFrameSize; iBin++) {

            obj->factor[iBin] = -2.0f * M_PI * ((float) iBin) / ((float) ((halfFrameSize-1) * 2));

        }

        obj->speed = ((float) obj->fS) / obj->c;

        return obj;

    }

    void track2steer_destroy(track2steer_obj * obj) {

        unsigned int iBin;

        free((void *) obj->factor);
        free((void *) obj->mics);

        free((void *) obj);

    }

    void track2steer_process_demixing(track2steer_obj * obj, const tracks_obj * tracks, const gains_obj * gains, const masks_obj * masks, steers_obj * steers) {

        unsigned int iSep;
        unsigned int iBin;
        unsigned int iChannel;
        
        float dist;
        float delay;
        float gain;
        float gainTotal;
        char mask;
        
        float Wreal;
        float Wimag;

        unsigned int iSampleSC;
        unsigned int iSampleBC;        

        for (iSep = 0; iSep < obj->nSeps; iSep++) {

            if (tracks->ids[iSep] != 0) {

                gainTotal = 0.0f;

                for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {

                    iSampleSC = iSep * obj->nChannels + iChannel;
                    mask = masks->array[iSampleSC];

                    if (mask == 1) {

                        gain = gains->array[iSampleSC];
                        gainTotal += gain * gain;

                    }

                }

                for (iChannel = 0; iChannel < obj->nChannels; iChannel++) {

                    iSampleSC = iSep * obj->nChannels + iChannel;
                    mask = masks->array[iSampleSC];

                    if (mask == 1) {                   

                        dist = tracks->array[iSep * 3 + 0] * obj->mics[iChannel * 3 + 0] + 
                               tracks->array[iSep * 3 + 1] * obj->mics[iChannel * 3 + 1] +
                               tracks->array[iSep * 3 + 2] * obj->mics[iChannel * 3 + 2]; 

                        delay = -1.0f * obj->speed * dist;

                        gain = gains->array[iSampleSC] / gainTotal;

                        for (iBin = 0; iBin < obj->halfFrameSize; iBin++) {

                            iSampleBC = iBin * obj->nChannels + iChannel;

                            Wreal = gain * cosf(obj->factor[iBin] * -1.0f * delay);
                            Wimag = gain * sinf(obj->factor[iBin] * -1.0f * delay);

                            steers->array[iSep][iSampleBC * 2 + 0] = Wreal;
                            steers->array[iSep][iSampleBC * 2 + 1] = Wimag;

                        }

                    }

                }

            }
            else {

                memset(steers->array[iSep], 0x00, sizeof(float) * obj->halfFrameSize * obj->nChannels * 2);

            }

        }

    }