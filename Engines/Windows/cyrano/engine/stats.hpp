//
// Cyrano Chess engine
//
// Copyright (C) 2007  Harald JOHNSEN
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

class Stats {
public:
    Stats() {}
    void raz() {
        Hcutoff = Halpha = Hbeta = 0;
        Hcollide = Hdepthinsert = Hdepthfound = Hused = Hupdate = 0;
        Killers = reSearch = repCount = badType = counterHit = 0;
        NM1 = NM2 = NM3 = 0;
        LMR = LMRreSearch = LMRhint = 0;
        cCUT = cCUTrank1 = cCUTrankn = 0;
        // Hsize = 0; total hash, don't reset
        FP_frontier = FP_QS = QS_count = FP_SIB = 0;
        recapture = 0;
        cEval = cFullEval = cSafetyChange = 0;
        c_pv = c_all = c_int = c_pv_succ = c_alpha_cut = c_iid = c_iidbad = 0;
        c_threat = c_extcheck = c_bmext = 0;
        c_ttmove = c_recog = c_badscore = 0;
        cCUTa = cCUTaRank = 0;
        c_cut_H = c_cut_killer = c_cut_capt = c_cut_badcapt = c_cut_quies = 0;
        evc_hit = 0;
        c_altMoves = c_fmext = c_nmext = 0;
        hp_hit = hp_eval = qs_sp = forced_detect = forced_played = forced_ext = 0;
        c_razor = c_razor_nok = 0;
        qs_sp_beta = c_exp_alpha = c_tot_alpha = 0;
    }
    void report() {
//        printf("# Hcut=%d Ha=%d Hb=%d (%1.1f) FP=%d FPQS=%d rec=%d K=%d reS=%d lmr=%d lmrR=%d\n",
//            Hcutoff, Halpha, Hbeta, (100.0f*Hused) / Hsize, FP_frontier, FP_QS, recapture, Killers, reSearch, 
//            LMR, LMRreSearch);
        printf("# Hcut=%d %d %1.1f %1.1f %1.1f lmr=%d(%d) lmrr=%d (%1.1f) raz=%d/%d FP=%d FPQS=%d rec=%d K=%d reS=%d inQS=%d (%d,%d,%d) counter=%d\n",
            Hcutoff, Hcollide, (100.0f*(cCUTrank1+1)) / (cCUT+1), (100.0f*(cCUTrankn+1)) / (cCUT+1), 
            (100.0f*(cCUTaRank)) / (cCUTa+1),
            LMR, LMRhint, LMRreSearch, (100.0f*Hused) / Hsize, c_razor, c_razor_nok, FP_frontier, FP_QS,
            recapture, Killers, reSearch, QS_count, cEval - cFullEval, cFullEval, evc_hit, counterHit);
        printf("# n.pv=%d pv.s=%d all=%d int=%d a_cut=%d b_cut=%d iid=%d (%d) nm1=%d nmb=%d nmr=%d ex+=%d fm=%d nmthr=%d recog=%d bm=%d qs_beta=%d\n", 
            c_pv, c_pv_succ, c_all, c_int, c_alpha_cut, cCUT, c_iid, c_iidbad, NM1, NM2, NM3, c_extcheck, c_fmext, c_nmext,
            c_recog, c_bmext, qs_sp_beta);
        fflush(stdout);
    }
    void report2() {
        printf("# cut_H=%d cut_killer=%d cut_capt=%d cut_badcapt=%d cut_quies=%d "
            "bad=%d altMoves=%f fm=%d nmext=%d hphit=%d hpeval=%d sp=%d alpha exp=%d tot=%d\n", 
            c_cut_H, c_cut_killer, c_cut_capt, c_cut_badcapt, c_cut_quies, c_badscore,
            c_altMoves * 1.0f / (1+c_alpha_cut), c_fmext, c_nmext, hp_hit, hp_eval, qs_sp,
            c_exp_alpha, c_tot_alpha);
        fflush(stdout);
    }

    int Hcutoff, Halpha, Hbeta;
    int Hcollide, Hdepthinsert, Hdepthfound, Hused, Hupdate;
    int Killers, reSearch, badType, counterHit;
    int repCount;
    int cCUT, cCUTrank1, cCUTrankn, cCUTa, cCUTaRank;
    int NM1, NM2, NM3;
    int LMR, LMRreSearch, LMRhint;
    int Hsize;
    int FP_frontier, FP_QS, QS_count, FP_SIB;
    int c_razor, c_razor_nok;
    int recapture;
    int cEval, cFullEval, cSafetyChange;
    int c_pv, c_all, c_int, c_pv_succ, c_alpha_cut, c_iid, c_iidbad;
    int c_threat, c_extcheck, c_bmext;
    int c_ttmove;
    int c_recog;
    int c_cut_H, c_cut_killer, c_cut_capt, c_cut_badcapt, c_cut_quies, c_badscore;
    int evc_hit, c_altMoves, c_fmext, c_nmext;
    int hp_hit, hp_eval;
    int qs_sp, forced_detect, forced_played, forced_ext;
    int qs_sp_beta, c_exp_alpha, c_tot_alpha;
};

extern Stats stats;
