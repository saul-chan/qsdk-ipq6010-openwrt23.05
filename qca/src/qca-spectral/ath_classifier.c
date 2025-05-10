/*
 * =====================================================================================
 *
 *       Filename:  ath_classifier.c
 *
 *    Description:  Classifier
 *
 *        Version:  1.0
 *        Created:  12/26/2011 11:16:31 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Anil Hebbar (Algorithm)
 *         Author:  S.Karthikeyan
 *        Company:  Qualcomm Atheros
 *
 *        Copyright (c) 2012-2021 Qualcomm Technologies, Inc.
 *
 *        All Rights Reserved.
 *        Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 *        2012-2016 Qualcomm Atheros, Inc.
 *
 *        All Rights Reserved.
 *        Qualcomm Atheros Confidential and Proprietary.
 *
 * =====================================================================================
 */

#include "stdio.h"
#include "stdlib.h"
#include <linux/types.h>
#include <string.h>
#include <math.h>
#include "ath_classifier.h"

CLASSIFER_DATA_STRUCT class_data[CLASSIFIER_HASHSIZE];
float powf_precompute_table[256];

static void init_powf_table(void);

/*
 * Function     : init_classifier_lookup_tables
 * Description  : Initialize all look up tables, should be called first
 * Input params : Void
 *
 */
void init_classifier_lookup_tables(void)
{
    init_powf_table();
}


/*
 * Function     : init_classifier_data
 * Description  : Initializes the classifier data structure for given MAC address
 * Input params : MAC address, pointer to Spectral capabilities information,
 *                length of Spectral capabilities information
 */
void init_classifier_data(const u_int8_t* macaddr, void *spectral_caps,
        size_t spectral_caps_len)
{
    int index = 0;
    CLASSIFER_DATA_STRUCT *pclas = NULL;

    SPECTRAL_CLASSIFIER_ASSERT(macaddr != NULL);
    SPECTRAL_CLASSIFIER_ASSERT(spectral_caps != NULL);
    SPECTRAL_CLASSIFIER_ASSERT(spectral_caps_len == sizeof(pclas->caps));

    index = CLASSIFIER_HASH(macaddr);
    pclas = &class_data[index];

    memcpy(&(pclas->caps), spectral_caps, sizeof(pclas->caps));

    if (pclas->caps.advncd_spectral_cap == true) {
        pclas->thresholds.cw_int_det_thresh = ADVNCD_CW_INT_DET_THRESH;
        pclas->thresholds.wifi_det_min_diff = (int)ADVNCD_WIFI_DET_MIN_DIFF;
        pclas->thresholds.fhss_sum_scale_down_factor = ADVNCD_FHSS_SUM_SCALE_DOWN_FACTOR;
        pclas->thresholds.mwo_pwr_variation_threshold = ADVNCD_MWO_POW_VARTIATION_THRESH;
    } else {
        pclas->thresholds.cw_int_det_thresh = CW_INT_DET_THRESH;
        pclas->thresholds.wifi_det_min_diff = (int)WIFI_DET_MIN_DIFF;
        pclas->thresholds.fhss_sum_scale_down_factor = FHSS_SUM_SCALE_DOWN_FACTOR;
        pclas->thresholds.mwo_pwr_variation_threshold = MWO_POW_VARIATION_THRESH;
    }

    pclas->commit_done = false;
    pclas->spectral_log_first_time = true;
    pclas->t_bin_pwr = malloc(sizeof(u_int32_t) * MAX_NUM_BINS);
    SPECTRAL_CLASSIFIER_ASSERT(pclas->t_bin_pwr);
    memset(pclas->t_bin_pwr, 0, sizeof(u_int32_t) * MAX_NUM_BINS);
}

/*
 * Function     : get_classifier_data
 * Description  : Returns the classifier data structure for given MAC address
 * Input params : MAC address
 * Return       : Pointer to Classifier data structure
 *
 */
CLASSIFER_DATA_STRUCT* get_classifier_data(const u_int8_t* macaddr)
{
    int index = CLASSIFIER_HASH(macaddr);
    return &class_data[index];
}

/*
 * Function     : spectral_scan_log_data
 * Description  : Log the spectral data
 * Input params : Pointer to samp_msg, classifier data structure, mode, commit
 * Return       : Void
 */
static void
spectral_scan_log_data(struct spectral_samp_msg* msg, CLASSIFER_DATA_STRUCT *pclas,
                       DETECT_MODE mode, u_int32_t commit)
{
    u_int8_t *buf_ptr;
    struct spectral_samp_msg *msg_ptr;
    struct spectral_state_log_type *detection_state_ptr;
    u_int8_t span_id, detector_id;
    FILE* spectral_log_fp = NULL;

    /* Check if logging is active */
    if (!pclas->logging_active && !pclas->spectral_log_first_time)
        return;

    /* Check if the mode matches */
    if (mode == SPECT_CLASS_DETECT_NONE) {
        return;
    }

    /* Check if enough data has already been logged */
    if (pclas->commit_done) {
        return;
    }

    if (msg->signature != SPECTRAL_SIGNATURE) {
        fprintf(stderr, "Unexpected Spectral signature %u\n", msg->signature);
        return;
    }

    /* Check if the initialization needs to be done */
    if (pclas->spectral_log_first_time) {
        pclas->spectral_log_first_time = false;
        pclas->spectral_bin_bufSave = NULL;
        pclas->spectral_msg_buf = NULL;
        pclas->spectral_state_log = NULL;

        pclas->spectral_bin_bufSave = malloc(MAX_NUM_BINS *
                                             SPECTRAL_DBG_LOG_SAMP);
        if (!pclas->spectral_bin_bufSave) {
            fprintf(stderr, "Spectral Classifier: "
                    "Memory allocation for SAMP message logging failed\n");
            pclas->logging_active = false;
            goto fail;
        }
        memset(pclas->spectral_bin_bufSave, 0, (MAX_NUM_BINS *
                                                SPECTRAL_DBG_LOG_SAMP));

        pclas->spectral_msg_buf = malloc(sizeof(struct spectral_samp_msg) *
                                               SPECTRAL_DBG_LOG_SAMP);
        if (!pclas->spectral_msg_buf) {
            fprintf(stderr, "Spectral Classifier: "
                    "Memory allocation for SAMP message logging failed\n");
            pclas->logging_active = false;
            goto fail;
        }
        memset(pclas->spectral_msg_buf, 0, (sizeof(struct spectral_samp_msg) *
                                            SPECTRAL_DBG_LOG_SAMP));

        if (athssd_is_sample_state_logging_supported()) {
            pclas->spectral_state_log =
                malloc(sizeof(struct spectral_state_log_type) *
                       SPECTRAL_DBG_LOG_SAMP);
            if (!pclas->spectral_state_log) {
                fprintf(stderr, "Spectral Classifier: "
                        "Memory allocation for SAMP message logging failed\n");
                pclas->logging_active = false;
                goto fail;
            }
            memset(pclas->spectral_state_log, 0,
                   (sizeof(struct spectral_state_log_type) *
                    SPECTRAL_DBG_LOG_SAMP));
        }

        pclas->spectral_log_num_bin = MAX_NUM_BINS;
        pclas->spectral_num_samp_log = 0;
        pclas->last_samp = 0;
        pclas->logging_active = true;
    }

    if (!commit) {
        /* Get the offset to the place where the data needs to be saved */
        buf_ptr = pclas->spectral_bin_bufSave +
                  pclas->spectral_log_num_bin * pclas->last_samp;
        /* Copy the bins */
        memcpy(buf_ptr, msg->bin_pwr, pclas->spectral_log_num_bin);

        /* Get the offset to the place where the data needs to be saved */
        msg_ptr = pclas->spectral_msg_buf + pclas->last_samp;
        /* Copy the samp msg */
        memcpy(msg_ptr, msg, sizeof(struct spectral_samp_msg));

        if (athssd_is_sample_state_logging_supported()) {
            /* Get the offset to the place where the data needs to be saved */
            detection_state_ptr = pclas->spectral_state_log +
                                  pclas->last_samp;
            /* Copy the detection state */
            memcpy(detection_state_ptr, &pclas->detection_state,
                   sizeof(struct spectral_state_log_type));
        }

        pclas->spectral_num_samp_log++;

        if (pclas->spectral_num_samp_log >= SPECTRAL_DBG_LOG_SAMP ) {
            pclas->spectral_num_samp_log = SPECTRAL_DBG_LOG_SAMP;
            /* In case of log all samples, there is no trigger, commit */
            if (pclas->log_mode == SPECT_CLASS_DETECT_ALL) {
                commit = 1;
            }
        }

        pclas->last_samp++;

        if (pclas->last_samp == SPECTRAL_DBG_LOG_SAMP) {
            pclas->last_samp = 0;
        }
    }

    /* Check if enough samples have been captured. If so, log the data */
    if (commit) {

        /* Commit data to file */
        spectral_log_fp = fopen("classifier.log", "wt");
        u_int32_t version;

        /* Check and update LOG VERSION.
         * Classifier state information is to be written
         * while logging individual interference samples
         * such as MWO/CW/FHSS/WIFI.
         */
        if (athssd_is_sample_state_logging_supported() &&
            pclas->log_mode != SPECT_CLASS_DETECT_NONE) {
            version = SPECTRAL_LOG_VERSION_ID6;
        } else {
            version = SPECTRAL_LOG_VERSION_ID5;
        }

        if (!spectral_log_fp) {
            printf("Spectral Classifier: Could not open file %s to write\n",
                   "classifier.log");
            return;
        }

        printf("Spectral Classifier: Number of samples captured %zu\n",
               pclas->spectral_num_samp_log);

        printf("Spectral Classifier: Writing samples to file. Please wait for a \n"
               "                     few minutes. Classification functionality \n"
               "                     might be limited in the meantime...\n");
        {
            /* Print the data into a ascii file */
            size_t cnt = 0;
            bool is_165mhz_operation = false;
            uint64_t num_bytes_written = 0;
            int wcnt;
            char delimiter = ' ';
            int advncd_spectral = 0;
            size_t sampIdx = pclas->last_samp;

            if (IS_CHAN_WIDTH_80P80(msg->sscan_bw) && (msg->num_freq_spans == 3))
                is_165mhz_operation = true;

            if (pclas->spectral_num_samp_log < SPECTRAL_DBG_LOG_SAMP) {
                /* In this case, no wrap around, so the first sample is the start of the
                * buffer
                */
                sampIdx = 0;
            }

            wcnt = fprintf(spectral_log_fp,
                           "***** THIS IS A MACHINE GENERATED FILE, DO NOT EDIT *****\n\n");

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "version:%c%u\n",
                           delimiter, version);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "mode:%c%u\n",
                           delimiter, msg->spectral_mode);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "primary_frequency:%c%u\n",
                           delimiter, msg->pri20_freq);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "operating_channel_cfreq1:%c%u\n",
                           delimiter, msg->cfreq1);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "operating_channel_cfreq2:%c%u\n",
                           delimiter, msg->cfreq2);

            if (wcnt < 0) {
               perror("Error while writing to outFile");
               goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "sscan_cfreq1:%c%u\n",
                           delimiter, msg->sscan_cfreq1);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "sscan_cfreq2:%c%u\n",
                           delimiter, msg->sscan_cfreq2);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "operating_channel_width:%c%u\n",
                           delimiter, msg->operating_bw);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "sscan_channel_width:%c%u\n",
                           delimiter, msg->sscan_bw);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "mac_address:%c%s\n",
                           delimiter, ether_sprintf(pclas->macaddr));

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "gen3_linear_scaling:%c%u\n",
                           delimiter, 1);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "165mhz_operation:%c%u\n",
                           delimiter, is_165mhz_operation);

            if (wcnt < 0) {
                perror("Error while writing to file");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "\nspectral_caps\n\n");

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "phy_diag_cap:%c%u\n",
                           delimiter, pclas->caps.phydiag_cap);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "radar_cap:%c%u\n",
                           delimiter, pclas->caps.radar_cap);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "spectral_cap:%c%u\n",
                           delimiter, pclas->caps.spectral_cap);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "advanced_spectral_cap:%c%u\n",
                           delimiter, pclas->caps.advncd_spectral_cap);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "hw_gen:%c%u\n",
                           delimiter, pclas->caps.hw_gen);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "is_scaling_params_populated:%c%u\n",
                           delimiter, pclas->caps.is_scaling_params_populated);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "formula_id:%c%u\n",
                           delimiter, pclas->caps.formula_id);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "low_level_offset:%c%d\n",
                           delimiter, pclas->caps.low_level_offset);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "high_level_offset:%c%d\n",
                           delimiter, pclas->caps.high_level_offset);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "rssi_thr:%c%d\n",
                           delimiter, pclas->caps.rssi_thr);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "default_agc_max_gain:%c%u\n",
                           delimiter, pclas->caps.default_agc_max_gain);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "agile_spectral_cap:%c%u\n",
                           delimiter, pclas->caps.agile_spectral_cap);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "agile_spectral_cap_160:%c%u\n",
                           delimiter, pclas->caps.agile_spectral_cap_160);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "agile_spectral_cap_80p80:%c%u\n",
                           delimiter, pclas->caps.agile_spectral_cap_80p80);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "agile_spectral_cap_320:%c%u\n",
                           delimiter, pclas->caps.agile_spectral_cap_320);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "num_detectors_20mhz:%c%u\n",
                           delimiter, pclas->caps.num_detectors_20mhz);
            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "num_detectors_40mhz:%c%u\n",
                           delimiter, pclas->caps.num_detectors_40mhz);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "num_detectors_80mhz:%c%u\n",
                           delimiter, pclas->caps.num_detectors_80mhz);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "num_detectors_160mhz:%c%u\n",
                           delimiter, pclas->caps.num_detectors_160mhz);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "num_detectors_80p80mhz:%c%u\n",
                           delimiter, pclas->caps.num_detectors_80p80mhz);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "num_detectors_320mhz:%c%u\n",
                           delimiter, pclas->caps.num_detectors_320mhz);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            advncd_spectral = pclas->caps.advncd_spectral_cap;
            wcnt = fprintf(spectral_log_fp, "\nspectral_params\n\n");

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            if (advncd_spectral)
                wcnt = fprintf(spectral_log_fp, "num_spectral_params:%c%u\n",
                               delimiter, NUM_SPECTRAL_PARAMS_ADVANCED);
            else
                wcnt = fprintf(spectral_log_fp, "num_spectral_params:%c%u\n",
                               delimiter, NUM_SPECTRAL_PARAMS_NON_ADVANCED);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            if (!advncd_spectral) {
                wcnt = fprintf(spectral_log_fp, "fft_period:%c%u\n",
                               delimiter,
                               pclas->spectral_params.ss_fft_period);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;
            }

            wcnt = fprintf(spectral_log_fp, "scan_period:%c%u\n",
                           delimiter, pclas->spectral_params.ss_period);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "fft_recapture:%c%u\n",
                           delimiter,
                           pclas->spectral_params.ss_recapture);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp, "scan_count:%c%u\n",
                           delimiter, pclas->spectral_params.ss_count);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            if (!advncd_spectral) {
                wcnt = fprintf(spectral_log_fp, "short_report:%c%u\n",
                               delimiter,
                               pclas->spectral_params.ss_short_report);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;
            }

            wcnt = fprintf(spectral_log_fp, "priority:%c%u\n",
                           delimiter, pclas->spectral_params.ss_spectral_pri);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            if (advncd_spectral) {
                wcnt = fprintf(spectral_log_fp, "fft_size:%c%u\n",
                               delimiter, pclas->spectral_params.ss_fft_size);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "gc_ena:%c%u\n",
                               delimiter, pclas->spectral_params.ss_gc_ena);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "restart_ena:%c%u\n",
                               delimiter,
                               pclas->spectral_params.ss_restart_ena);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "noise_floor_ref:%c%d\n",
                               delimiter,
                               (int8_t)pclas->spectral_params.ss_noise_floor_ref);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "init_delay:%c%u\n",
                               delimiter,
                               pclas->spectral_params.ss_init_delay);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "nb_tone_thr:%c%u\n",
                               delimiter,
                               pclas->spectral_params.ss_nb_tone_thr);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "str_bin_thr:%c%u\n",
                               delimiter,
                               pclas->spectral_params.ss_str_bin_thr);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "wb_rpt_mode:%c%u\n",
                               delimiter,
                               pclas->spectral_params.ss_wb_rpt_mode);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "rssi_rpt_mode:%c%u\n",
                               delimiter,
                               pclas->spectral_params.ss_rssi_rpt_mode);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "rssi_thr:%c%d\n",
                               delimiter,
                               (int8_t)pclas->spectral_params.ss_rssi_thr);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "pwr_format:%c%u\n",
                               delimiter,
                               pclas->spectral_params.ss_pwr_format);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "rpt_mode:%c%u\n",
                               delimiter, pclas->spectral_params.ss_rpt_mode);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "bin_scale:%c%u\n",
                               delimiter, pclas->spectral_params.ss_bin_scale);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "dBm_adj:%c%u\n",
                               delimiter, pclas->spectral_params.ss_dbm_adj);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "chn_mask:%c%u\n",
                               delimiter, pclas->spectral_params.ss_chn_mask);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "frequency1:%c%u\n",
                               delimiter,
                               pclas->spectral_params.ss_frequency.cfreq1);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "frequency2:%c%u\n",
                               delimiter,
                               pclas->spectral_params.ss_frequency.cfreq2);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;

                wcnt = fprintf(spectral_log_fp, "sscan_bw:%c%u\n",
                               delimiter,
                               pclas->spectral_params.ss_bandwidth);

                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }

                num_bytes_written += wcnt;
            }

            wcnt = fprintf(spectral_log_fp, "\n");

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp,
                           "S.No %c Num-FFT-Bins %c"
                           "target_reset_count %c"
                           "num_freq_spans(M)\n",
                           delimiter, delimiter,
                           delimiter);

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            wcnt = fprintf(spectral_log_fp,
                           "   Span_id0 Span_id0_num_det(X)   " SPAN_DETECTION_STR "\n"
                           "      Detector_id0   " DET_INFO_STR "\n"
                           "      [Detector_id1   " DET_INFO_STR "]\n"
                           "       ... \n"
                           "      [Detector_id(X-1)   " DET_INFO_STR "]\n"
                           "   [Span_id1 Span_id1_num_det(Y)   " SPAN_DETECTION_STR "]\n"
                           "      [Detector_id0   " DET_INFO_STR "]\n"
                           "      [Detector_id1   " DET_INFO_STR "]\n"
                           "       ... \n"
                           "      [Detector_id(Y-1)   " DET_INFO_STR "]\n"
                           "    ... \n"
                           "    ... \n"
                           "    ... \n"
                           "   [Span_id(M-1) Span_id(M-1)_num_det(Z)   " SPAN_DETECTION_STR "]\n"
                           "      [Detector_id0   " DET_INFO_STR "]\n"
                           "      [Detector_id1   " DET_INFO_STR "]\n"
                           "       ... \n"
                           "      [Detector_id(Z-1)   " DET_INFO_STR "]\n");

            if (wcnt < 0) {
                perror("Error while writing to outFile");
                goto fail;
            }

            num_bytes_written += wcnt;

            for (cnt = 0; cnt < pclas->spectral_num_samp_log; cnt++) {
                size_t valCnt = 0;
                int is_pwr_format_enabled = 0;
                u_int8_t *buf_ptr;
                struct spectral_samp_msg *msg_ptr;
                struct spectral_state_log_type *detection_state_ptr;

                buf_ptr = pclas->spectral_bin_bufSave +
                              pclas->spectral_log_num_bin * sampIdx;

                msg_ptr = pclas->spectral_msg_buf + sampIdx;

                /* Write sample number */
                wcnt = fprintf(spectral_log_fp, "%zu %c ", cnt, delimiter);
                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }
                num_bytes_written += wcnt;

                /* Write bin power count */
                wcnt = fprintf(spectral_log_fp, "%u %c ",
                               msg->bin_pwr_count, delimiter);
                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }
                num_bytes_written += wcnt;

                /* Write target_reset_count */
                wcnt = fprintf(spectral_log_fp, "%u %c ",
                               msg_ptr->target_reset_count, delimiter);
                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }
                num_bytes_written += wcnt;

                /* Write num_freq_spans */
                wcnt = fprintf(spectral_log_fp, "%u %c ",
                               msg->num_freq_spans, delimiter);
                if (wcnt < 0) {
                    perror("Error while writing to outFile");
                    goto fail;
                }
                num_bytes_written += wcnt;

                /* Write new line */
                wcnt = fprintf(spectral_log_fp, "\n");
                   if (wcnt < 0) {
                       fprintf(stderr, "Error while writing to outFile\n");
                       goto fail;
                   }
                num_bytes_written += wcnt;

                /* Write span and detector info */
                for (span_id = 0; span_id < msg_ptr->num_freq_spans; span_id++) {
                    struct samp_freq_span_info *span = &msg_ptr->freq_span_info[span_id];

                    /* Write indent */
                    wcnt = fprintf(spectral_log_fp, "   ");

                    if (wcnt < 0) {
                        perror("Error while writing to outFile");
                        goto fail;
                    }
                    num_bytes_written += wcnt;

                    /* Write span_id and num_detectors for the span id */
                    wcnt = fprintf(spectral_log_fp, "%u %c %u %c",
                                   (span_id), delimiter,
                                   span->num_detectors,
                                   delimiter);
                    if (wcnt < 0) {
                        perror("Error while writing to outFile");
                        goto fail;
                    }
                    num_bytes_written += wcnt;

                    if (version == SPECTRAL_LOG_VERSION_ID6) {
                        /* Write detection state for the span */
                        detection_state_ptr = pclas->spectral_state_log + sampIdx;
                        int intf_type;

                        for (intf_type = 0; intf_type < SPECTRAL_SAMPLE_STATE_INTF_MAX;
                             intf_type++) {
                            wcnt = fprintf(spectral_log_fp, "0x%08X %c",
                                           detection_state_ptr->state_log[span_id][intf_type],
                                           delimiter);

                            if (wcnt < 0) {
                                perror("Error while writing to outFile");
                                goto fail;
                            }
                            num_bytes_written += wcnt;
                        }
                    }

                    /* Write new line */
                    wcnt = fprintf(spectral_log_fp, "\n");
                       if (wcnt < 0) {
                           fprintf(stderr, "Error while writing to outFile\n");
                           goto fail;
                       }
                    num_bytes_written += wcnt;

                    for (detector_id = 0; detector_id < span->num_detectors; detector_id++) {
                        struct samp_detector_info *det = &span->detector_info[detector_id];

                        /* Write indent */
                        wcnt = fprintf(spectral_log_fp, "      ");

                        if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                        }
                        num_bytes_written += wcnt;

                        /* Write detector_id */
                        wcnt = fprintf(spectral_log_fp, "%u %c",
                                       detector_id, delimiter);

                        if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                        }
                        num_bytes_written += wcnt;

                        /* Write start and end freq */
                        wcnt = fprintf(spectral_log_fp, "%u %c %u %c",
                                       det->start_frequency, delimiter,
                                       det->end_frequency, delimiter);

                        if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                        }
                        num_bytes_written += wcnt;

                        /* Write left edge bins info */
                        wcnt = fprintf(spectral_log_fp, "%u %c",
                                       det->left_edge_bins.num_bins,
                                       delimiter);

                        if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                        }
                        num_bytes_written += wcnt;

                        /* Write start_bin_idx for edge bins only if they are present */
                        if (det->left_edge_bins.num_bins) {
                            wcnt = fprintf(spectral_log_fp, "%u %c",
                                           det->left_edge_bins.start_bin_idx,
                                           delimiter);

                            if (wcnt < 0) {
                                perror("Error while writing to outFile");
                                goto fail;
                            }
                            num_bytes_written += wcnt;
                        }

                        /* Write start and end bin idx */
                        wcnt = fprintf(spectral_log_fp, "%u %c %u %c",
                                       det->start_bin_idx, delimiter,
                                       det->end_bin_idx, delimiter);

                        if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                        }
                        num_bytes_written += wcnt;

                        /* Write right edge bins info */
                        wcnt = fprintf(spectral_log_fp, "%u %c",
                                       det->right_edge_bins.num_bins,
                                       delimiter);

                        if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                        }
                        num_bytes_written += wcnt;

                        /* Write start_bin_idx for edge bins only if they are present */
                        if (det->right_edge_bins.num_bins) {
                            wcnt = fprintf(spectral_log_fp, "%u %c",
                                           det->right_edge_bins.start_bin_idx,
                                           delimiter);

                            if (wcnt < 0) {
                                perror("Error while writing to outFile");
                                goto fail;
                            }
                            num_bytes_written += wcnt;
                        }

                        /* Write the left edge bins */
                        if (det->left_edge_bins.num_bins) {
                            u_int16_t start_bin_idx = det->left_edge_bins.start_bin_idx;
                            u_int16_t end_bin_idx = start_bin_idx +
                                                    det->left_edge_bins.num_bins - 1;

                            for (valCnt = start_bin_idx; valCnt <= end_bin_idx; valCnt++) {
                                if (is_pwr_format_enabled)
                                    /* Write bin values, dbm format */
                                    wcnt = fprintf(spectral_log_fp, "%d %c ",
                                                   (int8_t)(buf_ptr[valCnt]),
                                                   delimiter);
                                else
                                    /* Write bin values, linear format */
                                    wcnt = fprintf(spectral_log_fp, "%u %c ",
                                                   (buf_ptr[valCnt]),
                                                   delimiter);
                                if (wcnt < 0) {
                                    perror("Error while writing to outFile");
                                    goto fail;
                                }
                            num_bytes_written += wcnt;
                            }
                        }

                        /* Write in-band bins */
                        for (valCnt = det->start_bin_idx; valCnt <= det->end_bin_idx; valCnt++) {
                            if (is_pwr_format_enabled)
                                /* Write bin values, dbm format */
                                wcnt = fprintf(spectral_log_fp, "%d %c ",
                                               (int8_t)(buf_ptr[valCnt]),
                                               delimiter);
                            else
                                /* Write bin values, linear format */
                                wcnt = fprintf(spectral_log_fp, "%u %c ",
                                               (buf_ptr[valCnt]),
                                               delimiter);
                            if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                            }
                            num_bytes_written += wcnt;
                        }

                        /* Write the right edge bins */
                        if (det->right_edge_bins.num_bins) {
                            u_int16_t start_bin_idx = det->right_edge_bins.start_bin_idx;
                            u_int16_t end_bin_idx = start_bin_idx +
                                                    det->right_edge_bins.num_bins - 1;

                            for (valCnt = start_bin_idx; valCnt <= end_bin_idx; valCnt++) {
                                if (is_pwr_format_enabled)
                                    /* Write bin values, dbm format */
                                    wcnt = fprintf(spectral_log_fp, "%d %c ",
                                               (int8_t)(buf_ptr[valCnt]),
                                               delimiter);
                                else
                                    /* Write bin values, linear format */
                                    wcnt = fprintf(spectral_log_fp, "%u %c ",
                                                   (buf_ptr[valCnt]),
                                                   delimiter);
                                if (wcnt < 0) {
                                    perror("Error while writing to outFile");
                                    goto fail;
                                }
                            num_bytes_written += wcnt;
                            }
                        }


                        /* Write tstamp and rssi */
                        wcnt = fprintf(spectral_log_fp, "%u %c %d %c",
                                       det->timestamp, delimiter,
                                       det->rssi, delimiter);

                        if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                        }
                        num_bytes_written += wcnt;

                        /* Write noise floor and agc_total_gain */
                        wcnt = fprintf(spectral_log_fp, "%d %c %u %c",
                                       det->noise_floor, delimiter,
                                       det->agc_total_gain,
                                       delimiter);

                        if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                        }
                        num_bytes_written += wcnt;

                        /* Write gainchange and pri80ind */
                        wcnt = fprintf(spectral_log_fp, "%u %c %u %c",
                                       det->gainchange, delimiter,
                                       det->pri80ind, delimiter);

                        if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                        }
                        num_bytes_written += wcnt;

                        /* Write raw_timestamp and timestamp_war_offset */
                        wcnt = fprintf(spectral_log_fp, "%u %c %u %c",
                                       det->raw_timestamp, delimiter,
                                       det->timestamp_war_offset,
                                       delimiter);

                        if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                        }
                        num_bytes_written += wcnt;

                        /* Write last_raw_timestamp and reset_delay */
                        wcnt = fprintf(spectral_log_fp, "%u %c %u %c",
                                       det->last_raw_timestamp,
                                       delimiter,
                                       det->reset_delay, delimiter);

                        if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                        }
                        num_bytes_written += wcnt;

                        /* Write blanking status */
                        wcnt = fprintf(spectral_log_fp, "%u %c",
                                       det->blanking_status, delimiter);
                        if (wcnt < 0) {
                            perror("Error while writing to outFile");
                            goto fail;
                        }
                        num_bytes_written += wcnt;

                        /* Write new line */
                        wcnt = fprintf(spectral_log_fp, "\n");
                        if (wcnt < 0) {
                           fprintf(stderr, "Error while writing to outFile\n");
                           goto fail;
                        }
                        num_bytes_written += wcnt;
                    }
                }

                sampIdx++;
                if (sampIdx == SPECTRAL_DBG_LOG_SAMP) {
                    sampIdx = 0;
                }

            }
        }
        printf("Spectral Classifier: Completed writing samples to file.\n");
fail:
        /* Cleanup */
        if (spectral_log_fp)
            fclose(spectral_log_fp);
        if (pclas->spectral_bin_bufSave)
            free(pclas->spectral_bin_bufSave);
        if (pclas->spectral_msg_buf)
            free(pclas->spectral_msg_buf);
        if (pclas->spectral_state_log)
            free(pclas->spectral_state_log);
        system("sync");
        pclas->commit_done = true;
    }
}

/*
 * Function     : init_powf_table
 * Description  : Initialize precomputed table for powf function
 * Input params : Void
 * Return       : Void
 */
static void init_powf_table(void)
{
    int16_t i = -128;

    for (; i <= 127; i++) {
        powf_precompute_table[((u_int8_t)i)] = powf((float)10.0, (float)(i / 20.0));
    }

    return;
}

/*
 * Function     : populate_detection_region_descriptor_str
 * Description  : Helper function to populate detection region descriptor string
 *                corresponding to the Spectral mode
 * Input params : Pointer to SAMP message, segment number (will be used only for
 *                normal mode, ignored for Agile mode), pointer to character
 *                array which should be populated, max size of character array
 *                to be populated (if the string to be written including the
 *                terminating '\0' cannot completely fit into the array, the
 *                function returns error. It is recommended that callers use a
 *                size of SPECT_DETECTION_FREQ_REGION_STR_MAXTOTALSIZE).
 * Return       : 0 on success, -1 on failure.
 */
static int populate_detection_region_descriptor_str(
        struct spectral_samp_msg *msg, u_int8_t freq_span_id,
        char *region_descriptor_str, size_t region_descriptor_str_size)
{
    int ret = 0;

    SPECTRAL_CLASSIFIER_ASSERT(NULL != msg);
    SPECTRAL_CLASSIFIER_ASSERT(NULL != region_descriptor_str);
    SPECTRAL_CLASSIFIER_ASSERT(0 != region_descriptor_str_size);

    switch(msg->spectral_mode)
    {
        case SPECTRAL_SCAN_MODE_NORMAL:
            ret = snprintf(region_descriptor_str, region_descriptor_str_size,
                    "%s %u", SPECT_DETECTION_FREQ_REGION_STR_NORMAL_MODE,
                    freq_span_id);

            if ((ret < 0) || (ret >= region_descriptor_str_size)) {
                return -1;
            } else {
                return 0;
            }

            break;
        case SPECTRAL_SCAN_MODE_AGILE:
            ret = snprintf(region_descriptor_str, region_descriptor_str_size,
                    "%s %u", SPECT_DETECTION_FREQ_REGION_STR_AGILE_MODE,
                    freq_span_id);

            if ((ret < 0) || (ret >= region_descriptor_str_size)) {
                return -1;
            } else {
                return 0;
            }

            break;
        default:
            return -1;
            break;
    }
}

/*
 * Function     : spectral_scan_classifer_sm_init
 * Description  : Initialize the Spectral Classifier State Machine. Init is done on type/mode
 * Input params : Pointer to classifier data, mode to initialize, freq span id
 * Return       : Void
 */
void
spectral_scan_classifer_sm_init(CLASSIFER_DATA_STRUCT *pclas, DETECT_MODE mode,
                                                    uint8_t freq_span_id)
{
    /* Initialize the microwave oven interference */
    if (mode & SPECT_CLASS_DETECT_MWO) {

        pclas->mwo[freq_span_id].burst_idx         = 0;
        pclas->mwo[freq_span_id].burst_found       = 0;
        pclas->mwo[freq_span_id].mwo_in_burst_time = 0;
        pclas->mwo[freq_span_id].thresh            = MWO_POW_VARIATION_THRESH;
        pclas->mwo[freq_span_id].rssi              = 0;
        pclas->mwo[freq_span_id].cur_bin           = 0;
        memset(&pclas->mwo[freq_span_id].mwo_param, 0, sizeof(spectral_mwo_param)*NUM_MWO_BINS);

    }

    /* Initialize the CW interference */
    if (mode & SPECT_CLASS_DETECT_CW) {
        pclas->cw[freq_span_id].burst_found        = 0;
        pclas->cw[freq_span_id].start_time         = 0;
        pclas->cw[freq_span_id].last_found_time    = 0;
        pclas->cw[freq_span_id].rssi               = 0;
        pclas->cw[freq_span_id].num_detected       = 0;
    }

    /* Initialize the WiFi interference */
    if (mode & SPECT_CLASS_DETECT_WiFi) {
        pclas->wifi[freq_span_id].num_detected = 0;
        pclas->wifi[freq_span_id].rssi = 0;
    }

    /* Initialize the FHSS interference */
    if (mode & SPECT_CLASS_DETECT_FHSS) {
        pclas->fhss[freq_span_id].cur_bin = 0;
        memset(&pclas->fhss[freq_span_id].fhss_param, 0, sizeof(spectral_fhss_param) *
               NUM_FHSS_BINS);
    }

    /* Initialize generic data */
    if (mode == SPECT_CLASS_DETECT_ALL) {
        /* Some generic init */
        pclas->spectral_detect_mode            = SPECT_CLASS_DETECT_ALL;
        pclas->wifi[freq_span_id].num_detected = 0;
        pclas->current_interference            = 0;
        pclas->mwo[freq_span_id].detect_ts     = 0;
        pclas->mwo[freq_span_id].num_detect    = 0;
        pclas->mwo[freq_span_id].found_mwo     = 0;
        pclas->cw[freq_span_id].detect_ts      = 0;
        pclas->cw[freq_span_id].num_detect     = 0;
        pclas->cw[freq_span_id].found_cw       = 0;
        pclas->wifi[freq_span_id].detect_ts    = 0;
        pclas->wifi[freq_span_id].num_detect   = 0;
        pclas->wifi[freq_span_id].found_wifi   = 0;
        pclas->fhss[freq_span_id].found_fhss   = 0;
        pclas->dsss_detect_ts                  = 0;
        pclas->dsss_num_detect                 = 0;
        pclas->cur_freq                        = 0;
        pclas->cur_agile_freq1                 = 0;
        pclas->cur_agile_freq2                 = 0;
        pclas->cw_cnt                          = 0;
        pclas->wifi_cnt                        = 0;
        pclas->mwo_cnt                         = 0;
        pclas->fhss_cnt                        = 0;
    }

    pclas->sm_init_done = TRUE;
}

/*
 * Function     : is_mwo_found_on_other_spans
 * Description  : Helper API to indicate if mwo is found on
                  any span other than the current span represented
                  by freq_span_id.
 * Input params : Pointer classifier data and freq span id.
 * Return       : true- found, flase -not found
 *
 */
static inline bool
is_mwo_found_on_other_spans(const CLASSIFIER_MWO_PARAMS *mwo, u_int8_t freq_span_id)
{
    u_int8_t span_id;

    for (span_id = 0; span_id < MAX_NUM_FREQ_SPANS; span_id++) {
        if (span_id != freq_span_id && mwo[span_id].found_mwo) {
            return true;
        }
    }

    return false;
}

/*
 * Function     : detect_mwo
 * Description  : Detect Microwave oven
 * Input params : Pointers to SAMP msg, Classifier data and span params
 * Return       : Found/Not Found
 *
 */
int detect_mwo(struct spectral_samp_msg* msg, CLASSIFER_DATA_STRUCT *pclas,
               struct samp_freq_span_params *span_prms)
{
    int mwo_burst_found     = 0;
    int mwo_device_found    = 0;
    char region_descriptor_str[SPECT_DETECTION_FREQ_REGION_STR_MAXTOTALSIZE] = {0};
    int rdesc_ret = 0;
    u_int8_t freq_span_id = span_prms->freq_span_id;
    u_int8_t *pfft_bins;
    CLASSIFIER_MWO_PARAMS *pspan = &pclas->mwo[freq_span_id];


    if (!(pclas->spectral_detect_mode & SPECT_CLASS_DETECT_MWO))  {
        return mwo_device_found;
    }

    /* Check if this is a valid Microwave Oven frequency */
    if (msg->pri20_freq < MWO_MIN_FREQ || msg->pri20_freq > MWO_MAX_FREQ) {
        return 0;
    }

    /* Currently, we do not support Agile Spectral in 2.4 GHz */
    if ((SPECTRAL_SCAN_MODE_AGILE == msg->spectral_mode)) {
        return 0;
    }

    pfft_bins = &msg->bin_pwr[span_prms->start_bin_idx];

    if (span_prms->rssi > GET_MWO_MIN_RSSI_THRESHOLD(pclas)) {
        /* There is something in the air */
        float rssiLin                   = powf_precompute_table[(u_int8_t)span_prms->rssi];
        u_int32_t bin_cnt               = 0;
        u_int32_t low_bnd_pwr           = 0;
        u_int32_t up_bnd_pwr            = 0;
        u_int32_t bin_group_diff_abs    = 0;
        /* Time delta since last recorded in-burst instance */
        u_int32_t time_since_in_burst   = 0;

        /* Calculate power levels for each bins */
        for (bin_cnt = 0; bin_cnt < span_prms->num_bins; bin_cnt++ ) {
            pclas->t_bin_pwr[bin_cnt] = (u_int32_t)(((float)pfft_bins[bin_cnt] * rssiLin) + 0.5);
        }

        for (bin_cnt = 0; bin_cnt < MWO_BIN_CLUSTER_COUNT; bin_cnt++) {
            low_bnd_pwr += pclas->t_bin_pwr[bin_cnt];
            up_bnd_pwr += pclas->t_bin_pwr[span_prms->num_bins - bin_cnt - 1];
        }

        bin_group_diff_abs = abs((int)(up_bnd_pwr - low_bnd_pwr));

        if (bin_group_diff_abs > GET_MWO_POW_VARIATION_THRESHOLD(pclas)) {
            pspan->thresh =
                (GET_PCLAS_MWO_TRHESHOLD(pspan) - (GET_PCLAS_MWO_TRHESHOLD(pspan) >> 2)) +
                (bin_group_diff_abs >> 2);
        }

        if (bin_group_diff_abs > (int)((GET_PCLAS_MWO_TRHESHOLD(pspan) * 7 ) >> 3) ) {
            /* Tune the threshold so that the threshold is not too low or too high */
            if (!IS_MWO_BURST_FOUND(pspan)) {
                /* First burst */
                mwo_update_sample_state(pclas, MWO_FIRST_BURST_DETECTED,
                                        freq_span_id);
                pspan->burst_start_time     = span_prms->tstamp;
                pspan->mwo_in_burst_time    = span_prms->tstamp;
                pspan->burst_found          = 1;
            } else {
                time_since_in_burst = span_prms->tstamp -
                    GET_PCLAS_MWO_IN_BURST_TIME(pspan);
                if (time_since_in_burst < MWO_INTER_BURST_DURATION) {
                    pspan->mwo_in_burst_time = span_prms->tstamp;
                    if ((span_prms->tstamp - GET_PCLAS_MWO_BURST_START_TIME(pspan)) >
                        MWO_MAX_BURST_TIME) {
                        /* This is not an MWO burst because it is on for too long */
                        mwo_update_sample_state(pclas, MWO_MAX_BURST_TIME_EXCEEDED,
                                                freq_span_id);
                        spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_MWO,
                                                        freq_span_id);
                    }
                } else {
                    if (GET_PCLAS_MWO_IN_BURST_TIME(pspan) ==
                                GET_PCLAS_MWO_BURST_START_TIME(pspan)) {
                        /* The burst is too short. Might be FHSS or some
                         * spurious signal.
                         */
#if CLASSIFIER_DEBUG
                        cinfo("Burst is too short. Might be spurious.\n");
#endif /* CLASSIFIER_DEBUG */
                        mwo_update_sample_state(pclas, MWO_BURST_TOO_SHORT,
                                                freq_span_id);
                        spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_MWO,
                                                        freq_span_id);
                    } else if (time_since_in_burst >
                            MWO_MAX_INTER_BURST_DURATION) {
                        /* The next burst occurred after too long a gap. Might be
                         * a spurious signal.
                         */
 #if CLASSIFIER_DEBUG
                        cinfo("Inter-burst duration is too high (%u us). Might "
                              "be spurious.\n",
                              time_since_in_burst);
#endif /* CLASSIFIER_DEBUG */
                        mwo_update_sample_state(pclas, MWO_INTER_BURST_DURATION_TOO_HIGH,
                                                freq_span_id);
                        spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_MWO,
                                                        freq_span_id);
                    } else {
                        mwo_update_sample_state(pclas, MWO_NEW_BURST_DETECTED,
                                                freq_span_id);
                        /* Previous burst is over, this could be a new burst */
                        pspan->burst_idx++;
                        pspan->rssi += span_prms->rssi;
                        if (pspan->burst_idx == MWO_NUM_BURST) {
                            mwo_update_sample_state(pclas, MWO_SUFFICIENT_BURSTS_FOR_DETECTION,
                                                    freq_span_id);
                            /* Got enough bursts to be sure */
                            pspan->rssi /= MWO_NUM_BURST;
                            mwo_burst_found = 1;
                            spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_MWO,
                                                            freq_span_id);
                        } else {
                            /* Start recording the new burst */
                            pspan->burst_start_time  = span_prms->tstamp;
                            pspan->mwo_in_burst_time = span_prms->tstamp;
                        }
                    }
                }
            }
        } else {
            mwo_update_sample_state(pclas,
                                    MWO_PULSE_PWR_VARIATION_W_DYN_THRESH_NOT_CROSSED,
                                    freq_span_id);
            if ((pspan->burst_found) &&
                 (bin_group_diff_abs < (int)(((GET_PCLAS_MWO_TRHESHOLD(pspan) * 7) >> 3) >> 2))) {
                /* We had found a burst. Check if the time is too long ago and clear it */
                if ((span_prms->tstamp - GET_PCLAS_MWO_BURST_START_TIME(pspan)) >
                    MWO_BURST_INACTIVITY_TIMEOUT) {
                    mwo_update_sample_state(pclas,
                        MWO_BURST_INACTIVITY_TIMEOUT_EXCEEDED_W_DYN_THRESH_NOT_CROSSED,
                        freq_span_id);
                    /* too long without a burst, clear microwave detection */
                    spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_MWO,
                                                    freq_span_id);
                }
            }
        }
    } else {
        mwo_update_sample_state(pclas, MWO_RSSI_INSUFFICIENT,
                                freq_span_id);
        /* No interference detected, check if we were looking for something */
        if (IS_MWO_BURST_FOUND(pspan)) {
            /* We had found a burst. Check if the time is too far back in time, and clear it */
            if ((span_prms->tstamp - GET_PCLAS_MWO_BURST_START_TIME(pspan)) >
                MWO_BURST_INACTIVITY_TIMEOUT) {
                mwo_update_sample_state(pclas, MWO_BURST_INACTIVITY_TIMEOUT_EXCEEDED_W_LOW_RSSI,
                                        freq_span_id);
                /* Too long without a burst, clear microwave detction */
                spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_MWO,
                                                freq_span_id);
            }
        }
    }

    if (mwo_burst_found) {
        /* Something has been detected. Introduce hysteresis for stability of detection */
        if (!pspan->found_mwo) {
            if (!pspan->num_detect) {
                mwo_update_sample_state(pclas, MWO_FIRST_DETECT_FOUND,
                                        freq_span_id);
                pspan->num_detect = 1;
                pspan->detect_ts = span_prms->tstamp;
            } else {
                mwo_update_sample_state(pclas, MWO_SUBSEQUENT_DETECT_FOUND,
                                        freq_span_id);
                pspan->num_detect++;
                if ((int)(span_prms->tstamp - GET_PCLAS_MWO_DETECT_TIMESTAMP(pspan)) <
                     (MWO_STABLE_DETECT_THRESH)) {
                    if(!is_mwo_found_on_other_spans(pclas->mwo, freq_span_id))
                        pclas->current_interference |= SPECT_CLASS_DETECT_MWO;
                    pspan->detect_ts = span_prms->tstamp;
                    rdesc_ret =
                            populate_detection_region_descriptor_str(msg, freq_span_id,
                               region_descriptor_str, sizeof(region_descriptor_str));
                    SPECTRAL_CLASSIFIER_ASSERT(0 == rdesc_ret);
                    mwo_update_sample_state(pclas, MWO_INTERFERENCE_FOUND,
                                            freq_span_id);
                    printf("Spectral Classifier: Found MWO Interference on %s\n",
                           region_descriptor_str);
                    mwo_device_found = 0;
                    pclas->mwo_cnt++;
                    /* Interference is detected, set commit flag now, to be used in logging
                     * at the end of the API
                     */
                    pclas->commit_log_type |= SPECT_CLASS_DETECT_MWO;
                    pspan->found_mwo = 1;
                } else {
                    mwo_update_sample_state(pclas, MWO_STABLE_DETECT_THRESH_EXCEEDED,
                                            freq_span_id);
                    /* Took too much time, reset the counter -- Should not be here */
                    pspan->num_detect = 1;
                    pspan->detect_ts = span_prms->tstamp;
                }
            }
        } else {
            /* Update the time */
            pspan->num_detect = 1;
            pspan->detect_ts = span_prms->tstamp;
        }
    } else if (pspan->found_mwo) {
        /* Check if it has been found before and not found for some time */
        if ((int)(span_prms->tstamp - pspan->detect_ts) > (MWO_DETECT_INACTIVITY_TIMEOUT)) {
            mwo_update_sample_state(pclas, MWO_DETECT_INACTIVITY_TIMEOUT_CROSSED,
                                    freq_span_id);
            if(!is_mwo_found_on_other_spans(pclas->mwo, freq_span_id))
                pclas->current_interference &=  ~(SPECT_CLASS_DETECT_MWO);
            pspan->num_detect = 0;

            rdesc_ret = populate_detection_region_descriptor_str(msg, freq_span_id,
                                region_descriptor_str,
                                sizeof(region_descriptor_str));

            SPECTRAL_CLASSIFIER_ASSERT(0 == rdesc_ret);
            printf("Spectral Classifier: No MWO Interferencei on %s\n",
                   region_descriptor_str);
            pspan->found_mwo = 0;
        }
    }

    return mwo_device_found;
}

/*
 * Function     : is_cw_found_on_other_spans
 * Description  : Helper API to indicate if cw is found on
                  any span other than the current span represented
                  by freq_span_id.
 * Input params : Pointer classifier data and freq span id.
 * Return       : true- found, flase -not found
 *
 */
static inline bool
is_cw_found_on_other_spans(const CLASSIFIER_CW_PARAMS *cw, u_int8_t freq_span_id)
{
    u_int8_t span_id;

    for (span_id = 0; span_id < MAX_NUM_FREQ_SPANS; span_id++) {
        if (span_id != freq_span_id && cw[span_id].found_cw) {
            return true;
        }
    }

    return false;
}

/*
 * Function     : detect_cw
 * Description  : Detect the CW interfernce.
 * Input params : Pointers to SAMP msg, classifier data and span params
 * Return       : Found/Not Found
 *
 */
int detect_cw(struct spectral_samp_msg* msg, CLASSIFER_DATA_STRUCT *pclas,
              struct samp_freq_span_params *span_prms)
{
    int ret_val = 0;
    u_int16_t num_bins;
    u_int8_t *pfft_bins;
    char region_descriptor_str[SPECT_DETECTION_FREQ_REGION_STR_MAXTOTALSIZE] =
                                                            {0};
    int rdesc_ret = 0;
    u_int8_t freq_span_id = span_prms->freq_span_id;
    CLASSIFIER_CW_PARAMS *pspan = &pclas->cw[freq_span_id];

    if (!(pclas->spectral_detect_mode & SPECT_CLASS_DETECT_CW))  {
        return ret_val;
    }

    num_bins = span_prms->num_bins;
    pfft_bins = &msg->bin_pwr[span_prms->start_bin_idx];

    /* Check if there is high noise floor */
    if (span_prms->rssi > GET_CW_RSSI_THRESH(pclas)) {
        u_int16_t bin_cnt   = 0;
        u_int16_t peak_bin  = 0;
        u_int16_t peak_val  = 0;
        u_int16_t chk_upr   = 0;
        u_int16_t chk_lwr   = 0;
        u_int16_t cw_centre_upr_bin   = 0;
        u_int16_t cw_centre_lwr_bin   = 0;
        u_int32_t upr_sum   = 0;
        u_int32_t lwr_sum   = 0;
        u_int32_t center_sum = 0;
        u_int16_t peak_bin_to_upr_bin_offset = 0;
        u_int16_t peak_bin_to_lwr_bin_offset = 0;
        u_int16_t dc_bin_start = (num_bins >> 1) - 1;
        u_int16_t dc_bin_end = dc_bin_start + 1;

        for (bin_cnt = 0; bin_cnt < num_bins; bin_cnt++ ) {
            if (peak_val < pfft_bins[bin_cnt]) {
                peak_bin = bin_cnt;
                peak_val = pfft_bins[bin_cnt];
            }
        }

        /* set the upper and lower bin markers */
        /* Note, this logic works only for GET_CW_INT_BIN_SUM_SIZE(pclas) = 3 */
        if ((peak_bin != (num_bins - 1) &&
             pfft_bins[peak_bin + 1] > pfft_bins[peak_bin] /GET_CW_PEAK_TO_ADJ_THRESH(pclas)) &&
            (peak_bin != 0 && pfft_bins[peak_bin - 1] >
             pfft_bins[peak_bin] /GET_CW_PEAK_TO_ADJ_THRESH(pclas))) {
            if (pfft_bins[peak_bin + 1] > pfft_bins[peak_bin - 1]) {
                peak_bin_to_lwr_bin_offset = 1;
                peak_bin_to_upr_bin_offset = 2;
            } else {
                peak_bin_to_lwr_bin_offset = 2;
                peak_bin_to_upr_bin_offset = 1;
            }
        } else if (peak_bin != (num_bins - 1) &&
                   pfft_bins[peak_bin + 1] > pfft_bins[peak_bin] / GET_CW_PEAK_TO_ADJ_THRESH(pclas)) {
            peak_bin_to_lwr_bin_offset = 1;
            peak_bin_to_upr_bin_offset = 2;
        } else if (peak_bin != 0 && pfft_bins[peak_bin - 1] >
                   pfft_bins[peak_bin] / GET_CW_PEAK_TO_ADJ_THRESH(pclas)) {
            peak_bin_to_lwr_bin_offset = 2;
            peak_bin_to_upr_bin_offset = 1;
        } else {
            peak_bin_to_lwr_bin_offset = 1;
            peak_bin_to_upr_bin_offset = 1;
        }

        if ((int)peak_bin - (int)peak_bin_to_lwr_bin_offset < 0) {
            cw_centre_lwr_bin = 0;
            cw_centre_upr_bin = cw_centre_lwr_bin + peak_bin_to_lwr_bin_offset +
                                peak_bin_to_upr_bin_offset;
        } else if (peak_bin + peak_bin_to_upr_bin_offset > (num_bins - 1)) {
            cw_centre_upr_bin = num_bins - 1;
            cw_centre_lwr_bin = cw_centre_upr_bin - (peak_bin_to_lwr_bin_offset +
                                                     peak_bin_to_upr_bin_offset);
        } else {
            cw_centre_lwr_bin = peak_bin - peak_bin_to_lwr_bin_offset;
            cw_centre_upr_bin = peak_bin + peak_bin_to_upr_bin_offset;
        }

        SPECTRAL_CLASSIFIER_ASSERT(cw_centre_lwr_bin >= 0 &&
                                   cw_centre_lwr_bin <= (num_bins - 1));
        SPECTRAL_CLASSIFIER_ASSERT(cw_centre_upr_bin >= 0 &&
                                   cw_centre_upr_bin <= (num_bins - 1));

        /* Skip lwr_sum/upr_sum if upper/lower bins are falling in the
         * bins corresponding to the DC */
        if (cw_centre_upr_bin + GET_CW_INT_BIN_SUM_SIZE(pclas) <= (num_bins - 1) &&
            (cw_centre_upr_bin + 3 < dc_bin_start || dc_bin_end < cw_centre_upr_bin + 1)) {
            chk_upr = 1;
        }

        if ((int)cw_centre_lwr_bin - GET_CW_INT_BIN_SUM_SIZE(pclas) >= 0 &&
            (cw_centre_lwr_bin - 1 < dc_bin_start || dc_bin_end < cw_centre_lwr_bin - 3)) {
            chk_lwr = 1;
        }

        /* center_sum = sum of bins in the range [cw_centre_lwr_bin, cw_centre_upr_bin] */
        for (bin_cnt = cw_centre_lwr_bin; bin_cnt <= cw_centre_upr_bin; bin_cnt++)
            center_sum += pfft_bins[bin_cnt];

        if (chk_upr) {
            SPECTRAL_CLASSIFIER_ASSERT(cw_centre_upr_bin + 3 <= (num_bins - 1));
            upr_sum = pfft_bins[cw_centre_upr_bin + 1] +
                      pfft_bins[cw_centre_upr_bin + 2] +
                      pfft_bins[cw_centre_upr_bin + 3];
        }

        if (chk_lwr) {
            SPECTRAL_CLASSIFIER_ASSERT((int)cw_centre_lwr_bin - 3 >= 0);
            lwr_sum = pfft_bins[cw_centre_lwr_bin - 1] +
                      pfft_bins[cw_centre_lwr_bin - 2] +
                      pfft_bins[cw_centre_lwr_bin - 3];
        }

        /*
         * Check if this is greater than threhold
         * XXX : Note, this logic works only for GET_CW_INT_BIN_SUM_SIZE(pclas) = 3
         */
        if ( (lwr_sum < (center_sum >> GET_CW_SUM_SCALE_DOWN_FACTOR(pclas))) &&
             (upr_sum < (center_sum >> GET_CW_SUM_SCALE_DOWN_FACTOR(pclas))) &&
             (center_sum > GET_CW_INT_DET_THRESH(pclas))) {
                  cw_update_sample_state(pclas, CW_FOUND_LIKELY_BURST,
                                         freq_span_id);
                  /* Found a likely CW interference case */
                  pspan->num_detected++;
                  pspan->rssi += span_prms->rssi;

                  if (!pspan->burst_found) {
                      cw_update_sample_state(pclas, CW_FOUND_FIRST_BURST,
                                             freq_span_id);
                      pspan->burst_found       = 1;
                      pspan->start_time        = span_prms->tstamp;
                      pspan->last_found_time   = span_prms->tstamp;

                  } else {
                      cw_update_sample_state(pclas, CW_FOUND_SUBSEQUENT_BURST,
                                             freq_span_id);
                      pspan->last_found_time = span_prms->tstamp;

                      if (((int)(pspan->last_found_time - pspan->start_time) >
                            GET_CW_INT_FOUND_TIME_THRESH(pclas)) &&
                          (pspan->num_detected > GET_CW_INT_FOUND_MIN_CNT(pclas))) {
                          cw_update_sample_state(pclas, CW_FOUND_MIN_COUNT_BURST,
                                                 freq_span_id);
                          pspan->rssi /= pspan->num_detected;
                          spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_CW,
                                                          freq_span_id);
                          ret_val = 1;

                      }
                  }

             } else {

                 if (pspan->burst_found) {

                     if ((int)(span_prms->tstamp - pspan->last_found_time) >
                         GET_CW_INT_MISSING_THRESH(pclas) ) {
                         /* Burst missing for too long */
                         cw_update_sample_state(pclas,
                             CW_BURST_MISSING_FOR_TOO_LONG_W_CW_RSSI_THRESH_EXCEEDED,
                             freq_span_id);
                         spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_CW,
                                                         freq_span_id);
                     }
                 }
             }

    } else {

        if (pspan->burst_found) {

            /* Check how long the burst has been missing */
            if ((int)(span_prms->tstamp - pspan->last_found_time) >
                GET_CW_INT_MISSING_THRESH(pclas) ) {
                /* Burst missing for too long */
                cw_update_sample_state(pclas,
                    CW_BURST_MISSING_FOR_TOO_LONG_WO_CW_RSSI_EXCEEDED,
                    freq_span_id);
                spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_CW,
                                                freq_span_id);
            }
        }
    }

    if (ret_val) {

        /* add hystrysis to the detection in order to provide stablity */
        ret_val = 0;

        if (!pspan->found_cw) {

            if (!pspan->num_detect) {
                cw_update_sample_state(pclas, CW_FOUND_FIRST_DETECT,
                                       freq_span_id);
                pspan->num_detect = 1;
                pspan->detect_ts = span_prms->tstamp;

            } else {
                cw_update_sample_state(pclas, CW_FOUND_SUBSEQUENT_DETECT,
                                       freq_span_id);
                pspan->num_detect++;

                if (pspan->num_detect >=3) {
                    if ((int)(span_prms->tstamp - pspan->detect_ts) <
                        GET_CW_INT_CONFIRM_WIN(pclas)) {
                        cw_update_sample_state(pclas, CW_FOUND_SUFFICIENT_DETECTS_IN_WINDOW,
                                               freq_span_id);
                        /* found 2 detect within given window */
                        if(!is_cw_found_on_other_spans(pclas->cw, freq_span_id))
                            pclas->current_interference |= SPECT_CLASS_DETECT_CW;
                        pspan->detect_ts = span_prms->tstamp;
                        pclas->cw_cnt++;

                        rdesc_ret =
                            populate_detection_region_descriptor_str(msg, freq_span_id,
                                    region_descriptor_str,
                                    sizeof(region_descriptor_str));

                        SPECTRAL_CLASSIFIER_ASSERT(0 == rdesc_ret);

                        printf("Spectral Classifier: Found CW interference on %s\n",
                            region_descriptor_str);

                        /* Interference is detected, set commit value now, to be used in logging
                         * at the end of the API
                         */
                        pclas->commit_log_type |= SPECT_CLASS_DETECT_CW;

                        ret_val = 1;
                        pspan->found_cw = 1;
                    } else {
                        pspan->num_detect = 0;
                        pspan->detect_ts = span_prms->tstamp;
                    }
                }
            }
        } else {
            cw_update_sample_state(pclas, CW_FOUND_DETECT_AFTER_LONG_TIME,
                                   freq_span_id);
            /* Found after a long time */
            pspan->num_detect = 1;
            pspan->detect_ts = span_prms->tstamp;
        }

    } else if (pspan->found_cw) {

        /* Check if it has been found before and not found of some time */
        if ((int)(span_prms->tstamp - pspan->detect_ts) > GET_CW_INT_CONFIRM_MISSING_WIN(pclas)) {
            cw_update_sample_state(pclas, CW_FOUND_INSUFFICIENT_DETECTS_IN_WINDOW,
                                   freq_span_id);
            if(!is_cw_found_on_other_spans(pclas->cw, freq_span_id)) {
                pclas->current_interference &=  ~(SPECT_CLASS_DETECT_CW);
            }
            pspan->num_detect = 0;

            rdesc_ret =  populate_detection_region_descriptor_str(msg, freq_span_id,
                                region_descriptor_str,
                                sizeof(region_descriptor_str));

            SPECTRAL_CLASSIFIER_ASSERT(0 == rdesc_ret);

            printf("Spectral Classifier: No CW interference on %s\n",
                    region_descriptor_str);
            pspan->found_cw = 0;
        }
    }

    return ret_val;
}

/*
 * Function     : athssd_get_wifi_detection_state
 * Description  : Get the wifi sample state for given sscan bw, seg bw and seg id.
                  The API returns a 32 bit unsigned integer interpreted as below:
                  |........|........|........|........|
                  |   AA   |   BB   |   CC   |   DD   |
                  AA: sscan bw (stored as a factor WIFI_SEGMENT_BANDWIDTH_20MHZ
                      to ensure 1 byte limit)
                  BB: segment bw (stored as a factor WIFI_SEGMENT_BANDWIDTH_20MHZ
                      to ensure 1 byte limit)
                  CC: segment id
                  DD: miscellaneous state defined in SPECTRAL_WIFI_SAMPLE_STATE,
                      to be populated by the caller.
 * Input params : sscan_bw, seg bw, seg id
 * Return       : wifi sample state
 *
 */
static u_int32_t
athssd_get_wifi_detection_state(u_int32_t sscan_bw, u_int32_t seg_bw,
                                u_int8_t seg_id)
{
    return ((sscan_bw / WIFI_SEGMENT_BANDWIDTH_20MHZ) << WIFI_SSCAN_BW_SHIFT |
            (seg_bw / WIFI_SEGMENT_BANDWIDTH_20MHZ) << WIFI_SEG_BW_SHIFT |
            seg_id << WIFI_SEG_ID_SHIFT);
}

/*
 * Function     : is_wifi_found_on_other_spans
 * Description  : Helper API to indicate if wifi is found on
                  any span other than the current span represented
                  by freq_span_id.
 * Input params : Pointer classifier data and freq span id.
 * Return       : true- found, flase -not found
 *
 */
static inline bool
is_wifi_found_on_other_spans(const CLASSIFIER_WIFI_PARAMS *wifi, u_int8_t freq_span_id)
{
    u_int8_t span_id;

    for (span_id = 0; span_id < MAX_NUM_FREQ_SPANS; span_id++) {
        if (span_id != freq_span_id && wifi[span_id].found_wifi) {
            return true;
        }
    }

    return false;
}

/*
 * Function     : detect_wifi
 * Description  : Detect WiFi interference
 * Input params : Pointers to SAMP msg and classifier data, span params
 * Return       : Found/Not Found
 *
 * INFO         : WiFi interference is Wide Band signal. Low on the edge and should
 *                high enough RSSI
 *
 *
 * FFT Size initialization done before starting spectral scan
 * ----------------------------------------------------------
 * 20MHz    - FFT Size is set to 7, FFT bin count is 64 for 11ac, 56 for legacy
 * 40MHz    - FFT Size is set ti 8, FFT bin count is 128
 * 80MHz    - FFT Size is set ti 9, FFT bin count is 256
 */

int detect_wifi(struct spectral_samp_msg* msg, CLASSIFER_DATA_STRUCT* pclas,
                       struct samp_freq_span_params *span_prms)
{

    int found           = 0;
    u_int8_t *pfft_bins = NULL;
    u_int32_t num_bins_segment = 0;
    int seg_bw_id = 0;
    u_int32_t span_bw;
    u_int8_t num_segs;
    u_int32_t start_idx;
    unsigned int seg_bw, segment_id;
    u_int32_t wifi_state = WIFI_DETECT_INIT;
    WIFI_SEG_BANDWIDTH wifi_seg_bws[WIFI_NUM_SEG_BWS] = {WIFI_SEGMENT_BANDWIDTH_20MHZ,
                                                         WIFI_SEGMENT_BANDWIDTH_40MHZ,
                                                         WIFI_SEGMENT_BANDWIDTH_80MHZ,
                                                         WIFI_SEGMENT_BANDWIDTH_160MHZ,
                                                         WIFI_SEGMENT_BANDWIDTH_320MHZ};
    u_int8_t freq_span_id = span_prms->freq_span_id;
    CLASSIFIER_WIFI_PARAMS *pspan = &pclas->wifi[freq_span_id];
    char region_descriptor_str[SPECT_DETECTION_FREQ_REGION_STR_MAXTOTALSIZE] =
                                                            {0};
    int rdesc_ret = 0;

    if (!(pclas->spectral_detect_mode & SPECT_CLASS_DETECT_WiFi)) {
        return 0;
    }

    if (IS_CHAN_WIDTH_INVALID(msg->sscan_bw)) {
        /* ch_width is invalid, update sample state, stop further processing
         * and log the sample
         */
        wifi_update_sample_state(pclas, WIFI_CW_INVALID, freq_span_id);
        return 0;
    }

    span_bw = span_prms->span_bw;
    SPECTRAL_CLASSIFIER_ASSERT(span_bw);

    /* Check if there is a high signal */
    if (span_prms->rssi > GET_WIFI_DET_MIN_RSSI(pclas)) {
        pfft_bins = &msg->bin_pwr[span_prms->start_bin_idx];
        for (seg_bw_id = (WIFI_NUM_SEG_BWS - 1); seg_bw_id >= 0; seg_bw_id--) {
            seg_bw = wifi_seg_bws[seg_bw_id];
            if (seg_bw > span_bw)
                continue;
            num_segs = span_bw / seg_bw;
            num_bins_segment = span_prms->num_bins / num_segs;
            for (segment_id = 0; segment_id < num_segs; segment_id++) {
                start_idx = num_bins_segment * segment_id;
                /* Check for invalid start idx */
                SPECTRAL_CLASSIFIER_ASSERT((span_prms->start_bin_idx +
                                            start_idx + num_bins_segment - 1) <=
                                           span_prms->end_bin_idx);

                found = check_wifi_signal(pclas, num_bins_segment, pfft_bins + start_idx);
                if (found) {
                    wifi_state = athssd_get_wifi_detection_state(span_bw, seg_bw, segment_id + 1);
                    wifi_update_sample_state(pclas, wifi_state, freq_span_id);
                    break;
                }
            }
            if (found) {
#if CLASSIFIER_DEBUG
                if (IS_CHAN_WIDTH_80P80(msg->operating_bw))
                    cinfo("Detected WiFi (Channel Width 80p80 MHz : Span ID %u : %u MHz Segment %u)\n",
                          freq_span_id, seg_bw, segment_id + 1);
                else
                    cinfo("Detected WiFi (Channel Width %u MHz : Span ID %u : %u MHz Segment %u)\n",
                          span_bw, freq_span_id, seg_bw, segment_id + 1);
#endif
                break;
            }
        }
        /* If WiFi detected log the data */
        if (found) {
            pspan->num_detected++;
            pspan->rssi += span_prms->rssi;

        }
    }

    if (found == TRUE) {
        found = FALSE;

        /* Check if there are at least 2 detects in a 500ms interval */
        if (!pspan->found_wifi) {

            if (!pspan->num_detect) {

                /* First detect */
                wifi_update_sample_state(pclas, ((wifi_state & WIFI_MISC_STATE_MASK) |
                                         WIFI_FIRST_DETECT), freq_span_id);
                pspan->num_detect = 1;
                pspan->detect_ts = span_prms->tstamp;

            } else {

                pspan->num_detect++;

                /* 500ms has not elapsed */
                if ((int)(span_prms->tstamp - pspan->detect_ts) < GET_WIFI_DET_CONFIRM_WIN(pclas)) {

                    if (pspan->num_detect >= WIFI_MIN_NUM_DETECTS) {

                        /* And there are 2 detects, set the WiFi interference flag to 1 */
                        wifi_update_sample_state(pclas, ((wifi_state & WIFI_MISC_STATE_MASK) |
                                                 WIFI_SUFFICIENT_DETECTS_WITHIN_DETECTION_WINDOW),
                                                 freq_span_id);
                        if(!is_wifi_found_on_other_spans(pclas->wifi, freq_span_id))
                            pclas->current_interference |= SPECT_CLASS_DETECT_WiFi;
                        pspan->detect_ts       = span_prms->tstamp;
                        pspan->rssi /= pspan->num_detected;

                        rdesc_ret =
                           populate_detection_region_descriptor_str(msg, freq_span_id,
                                  region_descriptor_str, sizeof(region_descriptor_str));
                        SPECTRAL_CLASSIFIER_ASSERT(0 == rdesc_ret);

                        printf("Spectral Classifier: Found Wi-Fi interference on %s with RSSI %d\n",
                                region_descriptor_str, pspan->rssi);

                        pspan->found_wifi = 1;
                        found = 1;
                        pclas->wifi_cnt++;
                        /* Interference is detected, set commit value now, to be used in logging
                         * in the caller
                         */
                        pclas->commit_log_type |= SPECT_CLASS_DETECT_WiFi;
                    }
                } else {
                    /* Took too much time, reset the counter */
                    wifi_update_sample_state(pclas, ((wifi_state & WIFI_MISC_STATE_MASK) |
                                             WIFI_TOO_MUCH_TIME_FOR_SUFFICIENT_DETECTS),
                                             freq_span_id);
                    pspan->num_detect = 1;
                    pspan->detect_ts = span_prms->tstamp;
                }
            }
        } else {
            /* Check if a positive detect happend after a long time */
            if ((int)(span_prms->tstamp - pspan->detect_ts) > GET_WIFI_DET_RESET_TIME(pclas)) {
                wifi_update_sample_state(pclas, ((wifi_state & WIFI_MISC_STATE_MASK) |
                                         WIFI_POSITIVE_DETECT_AFTER_WIFI_DET_RESET_TIME),
                                         freq_span_id);
                /* Too much time, reset and check again */
                if(!is_wifi_found_on_other_spans(pclas->wifi, freq_span_id))
                    pclas->current_interference &=  ~(SPECT_CLASS_DETECT_WiFi);
                pspan->num_detect = 0;
                pspan->found_wifi = 0;

                rdesc_ret = populate_detection_region_descriptor_str(msg, freq_span_id,
                                  region_descriptor_str, sizeof(region_descriptor_str));
                SPECTRAL_CLASSIFIER_ASSERT(0 == rdesc_ret);

                printf("Spectral Classifier: No WiFi interference on %s\n",
                       region_descriptor_str);
            } else {
                wifi_update_sample_state(pclas, ((wifi_state & WIFI_MISC_STATE_MASK) |
                                         WIFI_ONE_MORE_DETETCED),
                                         freq_span_id);
                /* One more detected, reset the time */
                pspan->num_detect = 1;
                pspan->detect_ts = span_prms->tstamp;
            }
        }
    } else if (pspan->found_wifi) {
        /* Check if it has been found before and not found of some time */
        if ((int)(span_prms->tstamp - pspan->detect_ts) > GET_WIFI_DET_RESET_TIME(pclas)) {
            if(!is_wifi_found_on_other_spans(pclas->wifi, freq_span_id))
                pclas->current_interference &=  ~(SPECT_CLASS_DETECT_WiFi);
            pspan->num_detect = 0;
            wifi_update_sample_state(pclas, ((wifi_state & WIFI_MISC_STATE_MASK) |
                                     WIFI_NOT_DETECTED),
                                     freq_span_id);
            pspan->found_wifi = 0;

            rdesc_ret = populate_detection_region_descriptor_str(msg, freq_span_id,
                            region_descriptor_str, sizeof(region_descriptor_str));
            SPECTRAL_CLASSIFIER_ASSERT(0 == rdesc_ret);

            printf("Spectral Classifier: No WiFi interference on %s\n",
                   region_descriptor_str);
        }
    }

    return found;
}

/*
 * Function     : check_wifi_signal
 * Description  : checks for WiFi signal pattern in the given FFT bins
 * Input params : Pointers to classifier data, number of fft bins, pointer to fft bins
 * Return       : Found/Not Found
 *
 * INFO         : WiFi interference is Wide Band signal. Low on the edge and should
 *                high enough RSSI
 *
 */
int check_wifi_signal(CLASSIFER_DATA_STRUCT* pclas, u_int32_t num_bins, u_int8_t* pfft_bins)
{
    u_int16_t peak_val = 0;
    u_int16_t num_bins_above_threshold = 0;
    int i = 0;
    int found = FALSE;
    int peak_val_threshold = 0;

    /* find the peak value in the given bins */
    for (i = 0; i < num_bins; i++) {
        if (peak_val < pfft_bins[i]) {
            peak_val = pfft_bins[i];
        }
    }

    /* check how many bins are above peak bins are above threshold */
    /* Peak val threhold is set to 25% of peak value */
    peak_val_threshold = peak_val >> 2;

    for (i = 0; i < num_bins; i++) {
        if (pfft_bins[i] >= peak_val_threshold) {
            num_bins_above_threshold++;
        }
    }

    /* if at least half of the bins are greater than or equal to the peak value - 6dB */
    if (num_bins_above_threshold >= (num_bins >> 1)) {

        u_int32_t start_sum     = 0;
        u_int32_t mid_sum       = 0;
        u_int32_t end_sum       = 0;
        u_int32_t mid_bin_index = 0;

        mid_bin_index = (num_bins >> 1) - (GET_WIFI_BIN_WIDTH(pclas) >> 1) - 1;

        for (i = 0; i < (GET_WIFI_BIN_WIDTH(pclas)); i++) {
            start_sum   += pfft_bins[i];
            end_sum     += pfft_bins[num_bins - i -1];
            mid_sum     += pfft_bins[mid_bin_index + i - 1];
        }

        if (((int)(mid_sum - end_sum) > GET_WIFI_DET_MIN_DIFF(pclas)) &&
            ((int)(mid_sum - start_sum) > GET_WIFI_DET_MIN_DIFF(pclas))) {
            /* Most likely WiFi Signal */
            found = TRUE;
        }
    }

    return found;
}

/*
 * Function     : is_fhss_found_on_other_spans
 * Description  : Helper API to indicate if fhss is found on
                  any span other than the current span represented
                  by freq_span_id.
 * Input params : Pointer classifier data and freq span id.
 * Return       : true- found, flase -not found
 *
 */
static inline bool
is_fhss_found_on_other_spans(const CLASSIFIER_FHSS_PARAMS *fhss, u_int8_t freq_span_id)
{
    u_int8_t span_id;

    for (span_id = 0; span_id < MAX_NUM_FREQ_SPANS; span_id++) {
        if (span_id != freq_span_id && fhss[span_id].found_fhss) {
            return true;
        }
    }

    return false;
}

/*
 * Function     : detect_fhss
 * Description  : Detect FHSS Interference
 * Input params : Pointer to SAMP msg, classifier data and span params
 * Return       : Found/Not Found
 *
 * INFO         : FHSS in narrow band signal. Frequency hopping signals dwell for fixed amount
 *                of time (10ms) in single channel.
 *
 */
int detect_fhss(struct spectral_samp_msg* msg, CLASSIFER_DATA_STRUCT *pclas,
                struct samp_freq_span_params *span_prms)
{
    int ret_val = 0;
    u_int8_t *bin_pwr = NULL;
    char region_descriptor_str[SPECT_DETECTION_FREQ_REGION_STR_MAXTOTALSIZE] =
                                                            {0};
    int rdesc_ret = 0;
    u_int8_t freq_span_id = span_prms->freq_span_id;
    CLASSIFIER_FHSS_PARAMS *pspan = &pclas->fhss[freq_span_id];
    spectral_fhss_param *cur_fhss = &pspan->fhss_param[pspan->cur_bin];

    if (!(pclas->spectral_detect_mode & SPECT_CLASS_DETECT_FHSS)) {
        return ret_val;
    }

    bin_pwr = (u_int8_t *)&msg->bin_pwr[span_prms->start_bin_idx];

    if (span_prms->rssi <= GET_FHSS_DET_THRESH(pclas)) {
        fhss_update_sample_state(pclas, FHSS_RSSI_INSUFFICIENT,
                                 freq_span_id);
    }

    if ((span_prms->rssi > GET_FHSS_DET_THRESH(pclas)) &&
        (!(pclas->current_interference & SPECT_CLASS_DETECT_MWO))) {
        /* something in the air */
        u_int16_t bin_cnt       = 0;
        u_int16_t peak_bin      = 0;
        u_int16_t peak_val      = 0;
        u_int16_t chk_upr       = 0;
        u_int16_t chk_lwr       = 0;
        u_int16_t upr_bin       = 0;
        u_int16_t lwr_bin       = 0;
        u_int32_t upr_sum       = 0;
        u_int32_t lwr_sum       = 0;
        u_int32_t center_sum    = 0;
        u_int16_t peak_upper    = 0;
        u_int16_t peak_lower    = 0;

        /* Do a peak search and figure out the max data */
        for (bin_cnt = 0; bin_cnt < span_prms->num_bins; bin_cnt++ ) {
            if (peak_val < bin_pwr[bin_cnt]) {
                peak_bin = bin_cnt;
                peak_val = bin_pwr[bin_cnt];
            }
        }

        /* Check how many bins we can compare with */
        if (peak_bin + (GET_FHSS_INT_BIN_SUM_SIZE(pclas) +
            (GET_FHSS_INT_BIN_SUM_SIZE(pclas) >> 1)) <=
            span_prms->num_bins) {
            chk_upr = 1;
        }

        if ((int)peak_bin - (GET_FHSS_INT_BIN_SUM_SIZE(pclas) +
            (GET_FHSS_INT_BIN_SUM_SIZE(pclas) >> 1)) <= 0) {
            chk_lwr = 1;
        }


        /* XXX : Note, this logic works only for GET_FHSS_INT_BIN_SUM_SIZE(pclas) = 3 */
        /* set the upper and lower bin markers */
        if (peak_bin == span_prms->num_bins - 1) {
            upr_bin     = peak_bin;
            lwr_bin     = peak_bin - 2;
            peak_upper  = peak_bin;
            peak_lower  = peak_bin - 2;
        } else if (peak_bin == 0) {
            upr_bin     = peak_bin + 2;
            lwr_bin     = peak_bin;
            peak_upper  = peak_bin + 2;
            peak_lower  = peak_bin;
        } else {
            upr_bin     = peak_bin + 1;
            lwr_bin     = peak_bin - 1;
            peak_upper  = peak_bin + 2;
            peak_lower  = peak_bin - 2;
        }
        center_sum = bin_pwr[lwr_bin] +
                bin_pwr[lwr_bin + 1] +
                bin_pwr[lwr_bin + 2];
        if (chk_upr) {
                upr_sum = bin_pwr[upr_bin + 1] +
                        bin_pwr[upr_bin + 2] +
                        bin_pwr[upr_bin + 3];
        }
        if (chk_lwr) {
                lwr_sum = bin_pwr[lwr_bin - 1] +
                        bin_pwr[lwr_bin - 2] +
                        bin_pwr[lwr_bin - 3];
        }
        if (!(lwr_sum < (center_sum >> GET_FHSS_SUM_SCALE_DOWN_FACTOR(pclas))))
            fhss_update_sample_state(pclas, FHSS_LWR_SUM_TOO_HIGH,
                                     freq_span_id);
        if (!(upr_sum < (center_sum >> GET_FHSS_SUM_SCALE_DOWN_FACTOR(pclas))))
            fhss_update_sample_state(pclas, FHSS_UPR_SUM_TOO_HIGH,
                                     freq_span_id);
        if (!(center_sum > GET_FHSS_CENTER_THRESH(pclas)))
            fhss_update_sample_state(pclas, FHSS_CENTER_SUM_TOO_LOW,
                                     freq_span_id);

        /* Check if this is greater than threshold
         * XXX : Note, this logic works only for GET_FHSS_INT_BIN_SUM_SIZE(pclas) = 3
         */
        if ( (lwr_sum < (center_sum >> GET_FHSS_SUM_SCALE_DOWN_FACTOR(pclas))) &&
             (upr_sum < (center_sum >> GET_FHSS_SUM_SCALE_DOWN_FACTOR(pclas))) &&
             (center_sum > GET_FHSS_CENTER_THRESH(pclas)) ) {
            cur_fhss = &pspan->fhss_param[pspan->cur_bin];
            /* Possible FHSS burst */
            fhss_update_sample_state(pclas, FHSS_POSSIBLE_BURST,
                                     freq_span_id);

            /* Check if this condition has lasted long enough */
            if (!cur_fhss->in_use) {
                /* This is being used for the first time */
                cur_fhss->in_use    = 1;
                cur_fhss->start_ts  = span_prms->tstamp;
                cur_fhss->freq_bin  = peak_bin;
                cur_fhss->last_ts   = span_prms->tstamp;
                cur_fhss->rssi      += span_prms->rssi;
                cur_fhss->num_samp++;
            } else {
                /* This has been in use, check if this could be a new burst */
                if ( (cur_fhss->freq_bin < peak_upper) &&
                     (cur_fhss->freq_bin > peak_lower)) {
                    /* This is a current burst, check if this has been on for too long */
                    if ((int)(span_prms->tstamp - cur_fhss->start_ts) >
                        GET_FHSS_SINGLE_BURST_TIME(pclas)) {
                        /* This burst has been there for more then 15sec,
                         * it cannot be a FHSS burst
                         */
                        fhss_update_sample_state(pclas, FHSS_REJECTION_ABOVE_SINGLE_BURST_TIME,
                                                 freq_span_id);
                        spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_FHSS,
                                                        freq_span_id);
                    } else {
                        /* Store the last time stamp */
                        cur_fhss->last_ts   =  span_prms->tstamp;
                        cur_fhss->rssi      += span_prms->rssi;
                        cur_fhss->num_samp++;
                    }
                } else {
                    /* Try putting it in the next bin */
                    cur_fhss->delta = (int)(cur_fhss->last_ts - cur_fhss->start_ts);

                    /* If the delta is too short, just reuse the bin, it is a fake signal */
                    if (cur_fhss->delta > GET_FHSS_MIN_DWELL_TIME(pclas)) {
                        pspan->cur_bin++;
                        fhss_update_sample_state(pclas, FHSS_MIN_DWELL_TIME_SATISFIED,
                                                 freq_span_id);

                        if (pspan->cur_bin == NUM_FHSS_BINS) {
                            /* All bins are full, search to see if there is a possible FHSS */
                            u_int32_t avg_delta         = 0;
                            u_int32_t fir_delta         = pspan->fhss_param[1].delta;
                            u_int32_t fir_min           = fir_delta - (fir_delta >> 2);
                            u_int32_t fir_max           = fir_delta + (fir_delta >> 2);
                            u_int16_t num_fhss_burst    = 0;
                            u_int32_t tot_burst         = 0;
                            int32_t tot_rssi          = 0;

                            /* Check if the dwell time is about the minimum
                            * Optional: Not needed because already weeded out
                            */
                            if (fir_delta > GET_FHSS_MIN_DWELL_TIME(pclas)) {
                                /* loop through the dwell time and see if the if the dwell
                                 * time is about the same.
                                 */
                                for (bin_cnt = 0; bin_cnt < NUM_FHSS_BINS; bin_cnt++) {
                                    if ((pspan->fhss_param[bin_cnt].delta > fir_min) &&
                                        (pspan->fhss_param[bin_cnt].delta < fir_max)) {
                                        /* dwell time is about the same */
                                        avg_delta   += pspan->fhss_param[bin_cnt].delta;
                                        tot_rssi    += pspan->fhss_param[bin_cnt].rssi;
                                        tot_burst   += pspan->fhss_param[bin_cnt].num_samp;
                                        num_fhss_burst++;
                                    }
                                }

                                if (num_fhss_burst > (NUM_FHSS_BINS >> 1)) {
                                    /* At least 1/2 the bins have about the same dwell time
                                     * and hence declare FHSS burst detection.
                                     */
                                    avg_delta /= num_fhss_burst;
                                    tot_rssi /= tot_burst;
                                    ret_val = 1;
                                } else {
                                    fhss_update_sample_state(pclas,
                                        FHSS_INSUFFICIENT_BINS_W_SAME_DWELL,
                                        freq_span_id);
                                }
                            }else {
                                fhss_update_sample_state(pclas,
                                    FHSS_FIR_DELTA_NOT_CROSSING_MIN_DWELL,
                                    freq_span_id);
                            }
                            spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_FHSS,
                                                            freq_span_id);
                            fhss_update_sample_state(pclas, FHSS_REINIT_AFTER_ALL_BINS_PROCESSED,
                                                     freq_span_id);
                        }else {
                            fhss_update_sample_state(pclas, FHSS_NUM_REQD_VIABLE_PEAKS_NOT_REACHED,
                                                     freq_span_id);
                        }
                        /* Put the data back in the same bin */
                        if (pspan->cur_bin <= NUM_FHSS_BINS) {
                            cur_fhss = &pspan->fhss_param[pspan->cur_bin];
                            cur_fhss->in_use    = 1;
                            cur_fhss->start_ts  = span_prms->tstamp;
                            cur_fhss->freq_bin  = peak_bin;
                            cur_fhss->last_ts   = span_prms->tstamp;
                        } else {
                            printf("Spectral Classifier: Array out of bound error \n");
                        }
                    } else {
                        fhss_update_sample_state(pclas, FHSS_MIN_DWELL_TIME_NOT_SATISFIED,
                                                 freq_span_id);
                        /* add a new bin with different frequency */
                        cur_fhss = &pspan->fhss_param[pspan->cur_bin];
                        cur_fhss->in_use    = 1;
                        cur_fhss->start_ts  = span_prms->tstamp;
                        cur_fhss->freq_bin  = peak_bin;
                        cur_fhss->last_ts   = span_prms->tstamp;
                    }
                }
            }
        } else {
            /* Not a narrow band burst, check if it has been absent for too long */
            /* This is a current burst, check if this has been on for too long */
            if ((int)(span_prms->tstamp - cur_fhss->start_ts) >
                GET_FHSS_LACK_OF_BURST_TIME(pclas)) {
                /* This burst has been there for more then 15sec, it cannot be a FHSS burst */
                fhss_update_sample_state(pclas, FHSS_UNDETECTED_FOR_TOO_LONG_W_SUFFICIENT_RSSI,
                                         freq_span_id);
                spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_FHSS,
                                                freq_span_id);
            }
        }
    } else {
        /* No burst for a long time */
        /* This is a current burst, check if this has been on for too long */
        if ((int)(span_prms->tstamp - cur_fhss->start_ts) >
            GET_FHSS_LACK_OF_BURST_TIME(pclas)) {
            /* This burst has been there for more then 15sec, it cannot be a FHSS burst */
            fhss_update_sample_state(pclas, FHSS_UNDETECTED_FOR_TOO_LONG_W_LOW_RSSI,
                                     freq_span_id);
            spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_FHSS,
                                            freq_span_id);
        }
    }

    if (ret_val) {
        ret_val = 0;
        /* Check if at least 2 detects happen in given amount of time */
        if (!pspan->found_fhss) {
            if (!pspan->num_detect) {
                fhss_update_sample_state(pclas, FHSS_FIRST_DETECT_FOUND,
                                         freq_span_id);
                pspan->num_detect = 1;
                pspan->detect_ts  = span_prms->tstamp;
            } else {
                pspan->num_detect++;
                fhss_update_sample_state(pclas, FHSS_SUBSEQUENT_DETECT_FOUND,
                                         freq_span_id);
                if ((int)(span_prms->tstamp - pspan->detect_ts) <
                    GET_FHSS_DETECTION_CONFIRM_WIN(pclas)) {
                    if(!is_fhss_found_on_other_spans(pclas->fhss, freq_span_id))
                        pclas->current_interference |= SPECT_CLASS_DETECT_FHSS;
                    pspan->detect_ts = span_prms->tstamp;
                    rdesc_ret =
                        populate_detection_region_descriptor_str(msg, freq_span_id,
                            region_descriptor_str,
                            sizeof(region_descriptor_str));

                    SPECTRAL_CLASSIFIER_ASSERT(0 == rdesc_ret);

                    printf("Spectral Classifier: Found FHSS interference on %s\n",
                            region_descriptor_str);
                    pclas->fhss_cnt++;
                    pspan->found_fhss = 1;

                    /* Interference is detected, set commit flag now, to be used in logging
                     * at the end of the API
                     */
                    pclas->commit_log_type |= SPECT_CLASS_DETECT_FHSS;
                } else {
                    pspan->num_detect = 0;
                    pspan->detect_ts = span_prms->tstamp;
                    fhss_update_sample_state(pclas,
                        FHSS_NOT_SUFFICIENT_DETECTS_WITHIN_CONFIRMATION_WINDOW,
                        freq_span_id);
                }
            }
        } else {
            /* Update the time */
            pspan->num_detect = 1;
            pspan->detect_ts = span_prms->tstamp;
        }
    } else if (pspan->found_fhss) {
        /* Check if it has been found before and not found of some time */
        if ((int)(span_prms->tstamp - pspan->detect_ts) > GET_FHSS_DETECTION_RESET_WIN(pclas)) {
            if(!is_fhss_found_on_other_spans(pclas->fhss, freq_span_id))
                pclas->current_interference &=  ~(SPECT_CLASS_DETECT_FHSS);
            pspan->num_detect      = 0;
            rdesc_ret =  populate_detection_region_descriptor_str(msg, freq_span_id,
                                region_descriptor_str,
                                sizeof(region_descriptor_str));

            SPECTRAL_CLASSIFIER_ASSERT(0 == rdesc_ret);

            printf("Spectral Classifier: No FHSS interference on %s\n",
                    region_descriptor_str);
            pspan->found_fhss = 0;
        }
    }

    return ret_val;
}

/*
 * Function     : ether_sprintf
 * Description  : format MAC address for printing
 * Input params : pointer to mac address
 * Return       : formatted string
 *
 */

const char* ether_sprintf(const u_int8_t *mac)
{
    static char etherbuf[18];
    snprintf(etherbuf, sizeof(etherbuf), "%02x:%02x:%02x:%02x:%02x:%02x",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return etherbuf;
}

/*
 * Function     : get_span_max_rssi
 * Description  : get rssi in a freq span
 * Input params : Pointer to span_info
 * Return       : rssi
 *
 */
int16_t get_span_max_rssi(struct samp_freq_span_info *span_info)
{
    int16_t rssi = span_info->detector_info[0].rssi;
    int i;

    for (i = 1; i < span_info->num_detectors; i++) {
        rssi = max(rssi, span_info->detector_info[i].rssi);
    }

    return rssi;
}

/*
 * Function     : get_span_min_tstamp
 * Description  : get timestamp in a freq span
 * Input params : Pointer to span_info
 * Return       : timestamp
 *
 */
u_int64_t get_span_min_tstamp(struct samp_freq_span_info *span_info)
{
    u_int64_t tstamp = span_info->detector_info[0].timestamp;
    int i;

    for (i = 1; i < span_info->num_detectors; i++) {
        tstamp = min(tstamp, span_info->detector_info[i].timestamp);
    }

    return tstamp;
}

/*
 * Function     : get_span_start_bin_idx
 * Description  : get start bin index in a freq span
 * Input params : Pointer to span_info
 * Return       : start bin index
 *
 */
u_int16_t get_span_start_bin_idx(struct samp_freq_span_info *span_info)
{
    u_int16_t start_bin_idx = span_info->detector_info[0].start_bin_idx;
    int i;

    for (i = 1; i < span_info->num_detectors; i++) {
        start_bin_idx = min(start_bin_idx, span_info->detector_info[i].start_bin_idx);
    }

    return start_bin_idx;
}

/*
 * Function     : get_span_end_bin_idx
 * Description  : get end bin index in a freq span
 * Input params : Pointer to span_info
 * Return       : end bin index
 *
 */
u_int16_t get_span_end_bin_idx(struct samp_freq_span_info *span_info)
{
    u_int16_t end_bin_idx = span_info->detector_info[0].end_bin_idx;
    int i;

    for (i = 1; i < span_info->num_detectors; i++) {
        end_bin_idx = max(end_bin_idx, span_info->detector_info[i].end_bin_idx);
    }

    return end_bin_idx;
}

/*
 * Function     : get_span_start_frequency
 * Description  : get start frequency in a freq span
 * Input params : Pointer to span_info
 * Return       : start frequency
 *
 */
u_int32_t get_span_start_frequency(struct samp_freq_span_info *span_info)
{
    u_int32_t start_frequency = span_info->detector_info[0].start_frequency;
    int i;

    for (i = 1; i < span_info->num_detectors; i++) {
        start_frequency = min(start_frequency, span_info->detector_info[i].start_frequency);
    }

    return start_frequency;
}

/*
 * Function     : get_span_end_frequency
 * Description  : get end frequency in a freq span
 * Input params : Pointer to span_info
 * Return       : end frequency
 *
 */
u_int32_t get_span_end_frequency(struct samp_freq_span_info *span_info)
{
    u_int32_t end_frequency = span_info->detector_info[0].end_frequency;
    int i;

    for (i = 1; i < span_info->num_detectors; i++) {
        end_frequency = max(end_frequency, span_info->detector_info[i].end_frequency);
    }

    return end_frequency;
}

/*
 * Function     : print_spect_int_stats
 * Description  : Print spectral interference stats
 * Input params : Void
 * Return       : Void
 *
 */
extern void print_spect_int_stats()
{
    int i = 0;
    CLASSIFER_DATA_STRUCT *pclas;

    for (i = 0; i < CLASSIFIER_HASHSIZE; i++) {
        pclas = &class_data[i];

        if (pclas->is_valid) {
            printf("\nInterface = %s\n", ether_sprintf((const u_int8_t*)pclas->macaddr));
            printf("-----------------------------------------------\n");
            printf(" Number of MWO detection  %d\n"
                   " Number of WiFi detection %d\n"
                   " Number of FHSS detection %d\n"
                   " Number of CW detection   %d\n",
                    pclas->mwo_cnt,
                    pclas->wifi_cnt,
                    pclas->fhss_cnt,
                    pclas->cw_cnt);
        } /* end if */

    } /* end for */
}

/*
 * Function     : populate_span_info_params
 * Description  : Populate frequency span params
 * Input params : Pointers to freq span data structure, freq span params
 * Return       : status 0 for success, -1 for failure
 *
 */
static int
populate_span_info_params(struct samp_freq_span_info *span_info,
                          u_int8_t freq_span_id,
                          struct samp_freq_span_params *params)
{
    int status = 0;

    if (!span_info || !params) {
        status = -1;
        goto out;
    }

    params->rssi = get_span_max_rssi(span_info);
    params->tstamp = get_span_min_tstamp(span_info);
    params->start_bin_idx = get_span_start_bin_idx(span_info);
    params->end_bin_idx = get_span_end_bin_idx(span_info);
    params->num_bins = params->end_bin_idx - params->start_bin_idx + 1;
    params->start_frequency = get_span_start_frequency(span_info);
    params->end_frequency = get_span_end_frequency(span_info);
    params->span_bw = params->end_frequency - params->start_frequency;
    params->freq_span_id = freq_span_id;

out:
    return status;
}

/*
 * Function     : set_log_type
 * Description  : Set the type of info to log for debugging purpose
 * Input params : Pointer to classifier data structure, type of logging
 * Return       : Void
 *
 */
void set_log_type(CLASSIFER_DATA_STRUCT *pclas, u_int16_t log_type)
{
    switch (log_type) {
       case LOG_MWO:
           pclas->log_mode = SPECT_CLASS_DETECT_MWO;
           break;
       case LOG_CW:
           pclas->log_mode = SPECT_CLASS_DETECT_CW;
           break;
       case LOG_WIFI:
           pclas->log_mode = SPECT_CLASS_DETECT_WiFi;
           break;
       case LOG_FHSS:
           pclas->log_mode = SPECT_CLASS_DETECT_FHSS;
           break;
       case LOG_ALL:
           pclas->log_mode = SPECT_CLASS_DETECT_ALL;
           break;
       default:
           pclas->log_mode = SPECT_CLASS_DETECT_NONE;
           break;
    }
}

/*
 * Function     : get_scaled_bin_value
 * Description  : Wrapper function to get scaled bin value.
 * The scaled value can possibly be higher than 8 bits. This API
 * caps the scaled bin value to 255.
 * Input params : Bin value, scaling factor
 * Return       : Scaled bin value for bin index
 *
 */
static inline u_int32_t
get_scaled_bin_value(u_int32_t bin_value, double scaling_factor)
{
    u_int32_t scaled_bin_val;

    scaled_bin_val = bin_value * scaling_factor;

    scaled_bin_val = (scaled_bin_val > 255) ? 255 : scaled_bin_val;

    return scaled_bin_val;
}

/*
 * Function     : get_scaling_factor
 * Description  : Wrapper function to get scaling factor
 * Input params : rssi and agc_total_gain
 * Return       : scaling factor
 *
 */
static double
get_scaling_factor(int16_t rssi, u_int8_t agc_total_gain)
{
    double scaling_factor;

    scaling_factor =
        spectral_scaling_factor_linear_to_gen2(
            SPECTRAL_QCA9984_MAX_GAIN,
            SPECTRAL_IPQ8074_DEFAULT_MAX_GAIN_HARDCODE,
            SPECTRAL_SCALING_LOW_LEVEL_OFFSET,
            rssi,
            SPECTRAL_SCALING_RSSI_THRESH,
            agc_total_gain,
            SPECTRAL_SCALING_HIGH_LEVEL_OFFSET,
            1,
            1);

    return scaling_factor;
}

/*
 * Function     : do_bin_scaling
 * Description  : Wrapper function to scale bins in a range
 * Input params : Pointer to bin pwr array, start and end bin index,
                  rssi and agc_total_gain
 * Return       : void
 *
 */
static inline void
do_bin_scaling(u_int8_t *bin_pwr, u_int16_t start_bin_idx,
               u_int16_t end_bin_idx, int16_t rssi, u_int8_t agc_total_gain)
{
    u_int16_t bin_idx;
    double scaling_factor;

    scaling_factor = get_scaling_factor(rssi, agc_total_gain);
    for (bin_idx = start_bin_idx; bin_idx <= end_bin_idx; bin_idx++)
        bin_pwr[bin_idx] = get_scaled_bin_value(bin_pwr[bin_idx], scaling_factor);
}

/*
 * Function     : classifier_process_spectral_msg
 * Description  : Process the incoming SAMP message
 * Input params : Pointers to SAMP msg, classifier struct, log type, and whether
 *                to enable linear scaling for Gen3 chipsets
 * Return       : Void
 *
 */
void classifier_process_spectral_msg(struct spectral_samp_msg* msg,
        CLASSIFER_DATA_STRUCT *pclas, u_int16_t log_type,
        bool enable_gen3_linear_scaling)
{
    u_int8_t span_id;
    u_int8_t detector_id;

    /* validate */
    if (msg->signature != SPECTRAL_SIGNATURE) {
        return;
    }

    if (SPECTRAL_CAP_HW_GEN_3 == pclas->caps.hw_gen) {
        /* Check for gainchange and pri80ind */
        for (span_id = 0 ; span_id < msg->num_freq_spans; span_id ++) {
            struct samp_freq_span_info *span_info = &msg->freq_span_info[span_id];

            for (detector_id = 0; detector_id < span_info->num_detectors; detector_id++) {
                struct samp_detector_info *det_info = &span_info->detector_info[detector_id];

                /*
                 * For gen3 chipsets, the sample should be discarded if gain change is
                 * indicated.
                 */
                if (det_info->gainchange)
                    return;

                /*
                 * For gen3 chipsets, the sample should be discarded if hardware
                 * indicates that the sample was received on the primary 80 MHz segment
                 * instead of Agile frequency/secondary 80 MHz.
                 */
                if ((SPECTRAL_SCAN_MODE_AGILE == msg->spectral_mode) &&
                    det_info->pri80ind)
                    return;

                if ((SPECTRAL_SCAN_MODE_NORMAL == msg->spectral_mode) &&
                    det_info->pri80ind &&
                    det_info->is_sec80)
                    return;
            }
        }
    }

    /* Mark as valid */
    pclas->is_valid = TRUE;

    /* Store the interface mac address */
    memcpy(pclas->macaddr, msg->macaddr, MAC_ADDR_LEN);

    if (enable_gen3_linear_scaling &&
            (pclas->caps.hw_gen == SPECTRAL_CAP_HW_GEN_3)) {
        /*
         * Scale the gen3 bins to values approximately similar to those of
         * gen2.
         */

        /*
         * Note: Currently, we pass the same gen2 and gen3 bin_scale
         * values to the scaling formula, since for our algorithms we need
         * to scale for similar settings between gen2 and gen3. However, the
         * formula currently ignores the bin_scale values if they are the
         * same.
         * So we pass a bin_scale value of 1 for both gen2 and gen3 (which
         * is the recommended value for our algorithms anyway).
         *
         * A change can be added later to dynamically determine bin scale
         * values to be used.
         */

        /*
         * Optional: Get default max gain value, low level offset, RSSI
         * threshold, and high level offset from Spectral capabilities
         * structure once these are added there.
         */

        for (span_id = 0; span_id < msg->num_freq_spans; span_id ++) {
            struct samp_freq_span_info *span_info = &msg->freq_span_info[span_id];

            for (detector_id = 0; detector_id < span_info->num_detectors; detector_id++) {
                struct samp_detector_info *det_info = &span_info->detector_info[detector_id];
                struct samp_edge_extra_bin_info *lb_edge_bins = &det_info->left_edge_bins;
                struct samp_edge_extra_bin_info *rb_edge_bins = &det_info->right_edge_bins;
                u_int16_t start_bin_idx;
                u_int16_t end_bin_idx;

                /* Bin scaling for in-band bins */
                start_bin_idx = det_info->start_bin_idx;
                end_bin_idx = det_info->end_bin_idx;
                do_bin_scaling(&msg->bin_pwr[0],
                               start_bin_idx,
                               end_bin_idx,
                               det_info->rssi,
                               det_info->agc_total_gain);

                /* bin scaling for edge bins */

                /* Left edge bins */
                if (lb_edge_bins->num_bins) {
                    start_bin_idx = lb_edge_bins->start_bin_idx;
                    end_bin_idx = lb_edge_bins->start_bin_idx +
                                  lb_edge_bins->num_bins - 1;
                    do_bin_scaling(&msg->bin_pwr[0],
                                   start_bin_idx,
                                   end_bin_idx,
                                   det_info->rssi,
                                   det_info->agc_total_gain);
                }

                /* Right edge bins */
                if (rb_edge_bins->num_bins) {
                    start_bin_idx = rb_edge_bins->start_bin_idx;
                    end_bin_idx = rb_edge_bins->start_bin_idx +
                                  rb_edge_bins->num_bins - 1;
                    do_bin_scaling(&msg->bin_pwr[0],
                                   start_bin_idx,
                                   end_bin_idx,
                                   det_info->rssi,
                                   det_info->agc_total_gain);
                }
            }
        }
    }

    if (!pclas->sm_init_done) {
        /* Initialize the classifier state machine */
        for (span_id = 0; span_id < MAX_NUM_FREQ_SPANS; span_id++)
            spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_ALL, span_id);
        /* Set the log type */
        set_log_type(pclas, log_type);
    }

    /* Initialize the classifier state machine, if the frequency has changed */
    if (SPECTRAL_SCAN_MODE_NORMAL == msg->spectral_mode) {
        if (pclas->cur_freq != msg->pri20_freq) {
            for (span_id = 0; span_id < MAX_NUM_FREQ_SPANS; span_id++)
                spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_ALL, span_id);
            pclas->cur_freq = msg->pri20_freq;
        }
    } else if (SPECTRAL_SCAN_MODE_AGILE == msg->spectral_mode) {
        if (pclas->cur_agile_freq1 != msg->sscan_cfreq1 ||
            pclas->cur_agile_freq2 != msg->sscan_cfreq2) {
            for (span_id = 0; span_id < MAX_NUM_FREQ_SPANS; span_id++)
                spectral_scan_classifer_sm_init(pclas, SPECT_CLASS_DETECT_ALL, span_id);
            pclas->cur_agile_freq1 = msg->sscan_cfreq1;
            pclas->cur_agile_freq2 = msg->sscan_cfreq2;
        }
    }

    for (span_id = 0; span_id < MAX_NUM_FREQ_SPANS; span_id++) {
        mwo_update_sample_state(pclas, MWO_DETECT_INIT, span_id);
        cw_update_sample_state(pclas, CW_DETECT_INIT, span_id);
        wifi_update_sample_state(pclas, WIFI_DETECT_INIT, span_id);
        fhss_update_sample_state(pclas, FHSS_DETECT_INIT, span_id);
    }

    pclas->commit_log_type = 0;

    for (span_id = 0; span_id < msg->num_freq_spans; span_id++) {
        struct samp_freq_span_params params;
        int status;
        struct samp_freq_span_info *span_info = &msg->freq_span_info[span_id];

        status = populate_span_info_params(span_info, span_id, &params);
        SPECTRAL_CLASSIFIER_ASSERT(status == 0);
        if (params.span_bw == SAMP_SPAN_BW_5MHZ)
            continue;
        detect_mwo(msg, pclas, &params);
        detect_wifi(msg, pclas, &params);
        detect_cw(msg, pclas, &params);
        detect_fhss(msg, pclas, &params);
    }

    /* Log the Spectral data for debugging */
    spectral_scan_log_data(msg, pclas, pclas->log_mode, 0);
    if (pclas->log_mode != SPECT_CLASS_DETECT_ALL &&
        (pclas->commit_log_type & pclas->log_mode))
        spectral_scan_log_data(msg, pclas, pclas->log_mode, 1);
}

/*
 * Function     : print_detected_interference
 * Description  : Print the type of interference detected
 * Input params : Pointer to classifier struct
 * Return       : Void
 *
 */
void print_detected_interference(CLASSIFER_DATA_STRUCT* pclas)
{

    if (IS_MWO_DETECTED(pclas)) {
        printf("MWO Detected\n");
    }

    if (IS_CW_DETECTED(pclas)) {
        printf("CW Detected\n");
    }

    if (IS_WiFi_DETECTED(pclas)) {
        printf("WiFi Detected\n");
    }

    if (IS_CORDLESS_24_DETECTED(pclas)) {
        printf("CP (2.4GHZ) Detected\n");
    }

    if (IS_CORDLESS_5_DETECTED(pclas)) {
        printf("CP (5GHz) Detected\n");
    }

    if (IS_BT_DETECTED(pclas)) {
        printf("BT Detected\n");
    }

    if (IS_FHSS_DETECTED(pclas)) {
        printf("FHSS Detected\n");
    }

}


