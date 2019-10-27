// TODO replace with own implemenation

// generated automatically
// DO NOT MODIFY!
declare id "fuzzface";
declare name "Fuzz Face";
declare category "Fuzz";
declare description "J Hendrix Fuzz Face simulation";
declare insert_p "clipper";

import("stdfaust.lib");

fuzz = pre :  fi.iir((b0/a0,b1/a0,b2/a0,b3/a0,b4/a0,b5/a0),(a1/a0,a2/a0,a3/a0,a4/a0,a5/a0))   with {
    Inverted(b, x) = ba.if(b, 1 - x, x);
    s = 0.993;
    fs = float(ma.SR);
    pre = _;
   // clip = tranystage(TB_7199P_68k,86.0,2700.0,3.571981) : tranystage(TB_7199P_68k,86.0,2700.0,3.571981) ;
   
        Level = vslider("Level", 0.5, 0, 1, 0.01) : Inverted(1)  : si.smooth(s);
    
        Fuzz = vslider("Fuzz", 0.5, 0, 1, 0.01) : Inverted(1)  : si.smooth(s);
    
    b0 = Fuzz*(Fuzz*(Level*pow(fs,3)*(4.76991513499346e-20*fs + 5.38351707988916e-15) + pow(fs,3)*(-4.76991513499346e-20*fs - 5.38351707988916e-15)) + Level*pow(fs,3)*(-4.76991513499346e-20*fs + 5.00346713698171e-13) + pow(fs,3)*(4.76991513499346e-20*fs - 5.00346713698171e-13)) + Level*pow(fs,2)*(-5.05730339185222e-13*fs - 1.16162215422261e-12) + pow(fs,2)*(5.05730339185222e-13*fs + 1.16162215422261e-12);

    b1 = Fuzz*(Fuzz*(Level*pow(fs,3)*(-1.43097454049804e-19*fs - 5.38351707988916e-15) + pow(fs,3)*(1.43097454049804e-19*fs + 5.38351707988916e-15)) + Level*pow(fs,3)*(1.43097454049804e-19*fs - 5.00346713698171e-13) + pow(fs,3)*(-1.43097454049804e-19*fs + 5.00346713698171e-13)) + Level*pow(fs,2)*(5.05730339185222e-13*fs - 1.16162215422261e-12) + pow(fs,2)*(-5.05730339185222e-13*fs + 1.16162215422261e-12);

    b2 = Fuzz*(Fuzz*(Level*pow(fs,3)*(9.53983026998693e-20*fs - 1.07670341597783e-14) + pow(fs,3)*(-9.53983026998693e-20*fs + 1.07670341597783e-14)) + Level*pow(fs,3)*(-9.53983026998693e-20*fs - 1.00069342739634e-12) + pow(fs,3)*(9.53983026998693e-20*fs + 1.00069342739634e-12)) + Level*pow(fs,2)*(1.01146067837044e-12*fs + 2.32324430844522e-12) + pow(fs,2)*(-1.01146067837044e-12*fs - 2.32324430844522e-12);

    b3 = Fuzz*(Fuzz*(Level*pow(fs,3)*(9.53983026998693e-20*fs + 1.07670341597783e-14) + pow(fs,3)*(-9.53983026998693e-20*fs - 1.07670341597783e-14)) + Level*pow(fs,3)*(-9.53983026998693e-20*fs + 1.00069342739634e-12) + pow(fs,3)*(9.53983026998693e-20*fs - 1.00069342739634e-12)) + Level*pow(fs,2)*(-1.01146067837044e-12*fs + 2.32324430844522e-12) + pow(fs,2)*(1.01146067837044e-12*fs - 2.32324430844522e-12);

    b4 = Fuzz*(Fuzz*(Level*pow(fs,3)*(-1.43097454049804e-19*fs + 5.38351707988916e-15) + pow(fs,3)*(1.43097454049804e-19*fs - 5.38351707988916e-15)) + Level*pow(fs,3)*(1.43097454049804e-19*fs + 5.00346713698171e-13) + pow(fs,3)*(-1.43097454049804e-19*fs - 5.00346713698171e-13)) + Level*pow(fs,2)*(-5.05730339185222e-13*fs - 1.16162215422261e-12) + pow(fs,2)*(5.05730339185222e-13*fs + 1.16162215422261e-12);

    b5 = Fuzz*(Fuzz*(Level*pow(fs,3)*(4.76991513499346e-20*fs - 5.38351707988916e-15) + pow(fs,3)*(-4.76991513499346e-20*fs + 5.38351707988916e-15)) + Level*pow(fs,3)*(-4.76991513499346e-20*fs - 5.00346713698171e-13) + pow(fs,3)*(4.76991513499346e-20*fs + 5.00346713698171e-13)) + Level*pow(fs,2)*(5.05730339185222e-13*fs - 1.16162215422261e-12) + pow(fs,2)*(-5.05730339185222e-13*fs + 1.16162215422261e-12);

    a0 = Fuzz*(Fuzz*fs*(fs*(fs*(fs*(-3.73292075290073e-29*fs - 1.05633134620746e-20) - 3.11506369039915e-14) - 2.30719916990074e-11) - 1.07493164710329e-9) + fs*(fs*(fs*(fs*(3.73292075290073e-29*fs + 1.01643277726662e-20) + 2.91602352831988e-14) + 2.29636966370042e-11) + 1.07449105454163e-9)) + fs*(fs*(fs*(3.98985774247549e-22*fs + 1.99042653510896e-15) + 1.83615604104971e-13) + 5.31230624730483e-11) + 2.44402781742033e-9;

    a1 = Fuzz*(Fuzz*fs*(fs*(fs*(fs*(1.86646037645036e-28*fs + 3.16899403862238e-20) + 3.11506369039915e-14) - 2.30719916990074e-11) - 3.22479494130986e-9) + fs*(fs*(fs*(fs*(-1.86646037645036e-28*fs - 3.04929833179984e-20) - 2.91602352831988e-14) + 2.29636966370042e-11) + 3.22347316362488e-9)) + fs*(fs*(fs*(-1.19695732274265e-21*fs - 1.99042653510896e-15) + 1.83615604104971e-13) + 1.59369187419145e-10) + 1.22201390871017e-8;

    a2 = Fuzz*(Fuzz*fs*(fs*(fs*(fs*(-3.73292075290073e-28*fs - 2.11266269241492e-20) + 6.2301273807983e-14) + 4.61439833980148e-11) - 2.14986329420657e-9) + fs*(fs*(fs*(fs*(3.73292075290073e-28*fs + 2.03286555453323e-20) - 5.83204705663976e-14) - 4.59273932740084e-11) + 2.14898210908325e-9)) + fs*(fs*(fs*(7.97971548495099e-22*fs - 3.98085307021793e-15) - 3.67231208209942e-13) + 1.06246124946097e-10) + 2.44402781742033e-8;

    a3 = Fuzz*(Fuzz*fs*(fs*(fs*(fs*(3.73292075290073e-28*fs - 2.11266269241492e-20) - 6.2301273807983e-14) + 4.61439833980148e-11) + 2.14986329420657e-9) + fs*(fs*(fs*(fs*(-3.73292075290073e-28*fs + 2.03286555453323e-20) + 5.83204705663976e-14) - 4.59273932740084e-11) - 2.14898210908325e-9)) + fs*(fs*(fs*(7.97971548495099e-22*fs + 3.98085307021793e-15) - 3.67231208209942e-13) - 1.06246124946097e-10) + 2.44402781742033e-8;

    a4 = Fuzz*(Fuzz*fs*(fs*(fs*(fs*(-1.86646037645036e-28*fs + 3.16899403862238e-20) - 3.11506369039915e-14) - 2.30719916990074e-11) + 3.22479494130986e-9) + fs*(fs*(fs*(fs*(1.86646037645036e-28*fs - 3.04929833179984e-20) + 2.91602352831988e-14) + 2.29636966370042e-11) - 3.22347316362488e-9)) + fs*(fs*(fs*(-1.19695732274265e-21*fs + 1.99042653510896e-15) + 1.83615604104971e-13) - 1.59369187419145e-10) + 1.22201390871017e-8;

    a5 = Fuzz*(Fuzz*fs*(fs*(fs*(fs*(3.73292075290073e-29*fs - 1.05633134620746e-20) + 3.11506369039915e-14) - 2.30719916990074e-11) + 1.07493164710329e-9) + fs*(fs*(fs*(fs*(-3.73292075290073e-29*fs + 1.01643277726662e-20) - 2.91602352831988e-14) + 2.29636966370042e-11) - 1.07449105454163e-9)) + fs*(fs*(fs*(3.98985774247549e-22*fs - 1.99042653510896e-15) + 1.83615604104971e-13) - 5.31230624730483e-11) + 2.44402781742033e-9;
};

process = sp.stereoize(fuzz);