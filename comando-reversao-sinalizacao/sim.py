# Simulação booleana do comando, como li na foto (P = fase depois do S0)
def settle(st, s1, s0=False, t_done=False):
    for _ in range(20):
        P = not s0
        J = P                      # ponto 10 desce direto até a junção do ramo do K1
        B = P and s1               # barramento das linhas do Ka1/Ka2 só é alimentado pelo S1
        n = dict(st)
        n['K1'] = J and (st['Ka1'] or st['K1']) and not st['Ka2']
        n['Ka1'] = B and ((not st['K1']) or st['Ka1']) and not st['Ka2']
        # nó X = A1 do Ka2; nó Y = linha do Ka3/Kt1 (ligados pelo Ka2 83-84)
        X = B and (st['K1'] or st['Ka2']) and not st['Ka1']
        Y = P and st['Ka3']
        if st['Ka2']:
            X = X or Y; Y = Y or X
        n['Ka2'] = X
        n['Ka3'] = Y; n['Kt1'] = Y
        n['K2'] = X and st['Kt1'] and t_done
        n['_X'] = X
        if n == st: break
        st = n
    return st

st = dict(K1=False, Ka1=False, Ka2=False, Ka3=False, Kt1=False, K2=False, _X=False)
def show(nome, st):
    on = [k for k in ('K1','Ka1','Ka2','Ka3','Kt1','K2') if st[k]]
    led12 = not st['K1'] and not st['Ka3']
    buzz = st['_X'] and st['Kt1'] and not st['K2']      # Kt1 15-16 (NF temporizado) com comum no nó X
    led34 = st['K2']; led56 = st['K1']
    print(f"{nome:38s} ligados={on!s:32s} LED1/2={int(led12)} BUZ7/8={int(buzz)} LED3/4={int(led34)} LED5/6={int(led56)}")
show("energizou o painel", st := settle(st, False))
show("1º toque S1 (segurando)", st := settle(st, True))
show("soltou S1", st := settle(st, False))
show("2º toque S1 (segurando)", st := settle(st, True))
show("soltou S1 (temporizando)", st := settle(st, False))
show("fim do tempo do Kt1", st := settle(st, False, t_done=True))
show("apertou S0", st := settle(st, False, s0=True, t_done=True))
show("soltou S0", st := settle(st, False, t_done=False))
