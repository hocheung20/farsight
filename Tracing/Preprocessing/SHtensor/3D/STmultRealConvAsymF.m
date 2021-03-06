
function STres = STmult(ST1,ST2,J)

L1 = (size(ST1,1)/2-1);
L2 = (size(ST2,1)/2-1);

if J > L1+L2 | J < abs(L1-L2),
    disp('triangle condition violated!');
    return;
end;


cnt = 1;
for m = -J:0,
    for ms = -L1:L1,
        if abs(m-ms) <= L2,
            cg = ClebschGordan(L1,L2,J,ms,m-ms,m) ;
            idxlist(:,cnt) = [ms m-ms m cg];
            cnt = cnt + 1;
        end;
    end;
end;

STres = STmultRealFourierFast(ST1,ST2,J,myREAL(idxlist));

