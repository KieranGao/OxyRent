<template>
  <div class="auth-layout">
    <div class="auth-bg"></div>
    <div class="auth-content">
      <div class="auth-card">
        <div class="auth-brand">
          <div class="brand-mark">
            <svg width="32" height="32" viewBox="0 0 32 32" fill="none">
              <path d="M16 2L2 16L16 30L30 16L16 2Z" stroke="currentColor" stroke-width="1.5" fill="none"/>
              <path d="M16 8L8 16L16 24L24 16L16 8Z" fill="currentColor" opacity="0.3"/>
            </svg>
          </div>
          <h1 class="brand-name">OxyRent</h1>
          <p class="brand-tagline">车辆租赁管理系统</p>
        </div>
        <router-view />
      </div>
      <p class="auth-footer">2026 OxyRent. 精心打造</p>
    </div>
  </div>
</template>

<script setup>
</script>

<style scoped>
.auth-layout {
  min-height: 100vh;
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  background: #030303;
}

.auth-bg {
  position: absolute;
  inset: 0;
  background:
    radial-gradient(ellipse 80% 50% at 50% 0%, rgba(200, 169, 110, 0.06) 0%, transparent 60%),
    radial-gradient(ellipse 60% 40% at 80% 100%, rgba(200, 169, 110, 0.04) 0%, transparent 50%),
    radial-gradient(ellipse 40% 30% at 10% 60%, rgba(100, 100, 120, 0.05) 0%, transparent 50%);
  pointer-events: none;
}

.auth-content {
  position: relative;
  z-index: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  width: 100%;
  max-width: 420px;
  padding: 0 24px;
  animation: fadeIn 0.8s var(--ease-out);
}

.auth-card {
  width: 100%;
  padding: 48px 40px;
  background: var(--bg-surface);
  border: 1px solid var(--border);
  border-radius: var(--radius-xl);
  position: relative;
  overflow: hidden;
}

.auth-card::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 1px;
  background: linear-gradient(90deg, transparent, var(--accent), transparent);
}

.auth-brand {
  text-align: center;
  margin-bottom: 40px;
}

.brand-mark {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 56px;
  height: 56px;
  border: 1px solid var(--border-accent);
  border-radius: 12px;
  color: var(--accent);
  margin-bottom: 20px;
}

.brand-name {
  font-family: var(--font-display);
  font-size: 36px;
  font-weight: 400;
  color: var(--text-primary);
  letter-spacing: 2px;
  margin-bottom: 6px;
}

.brand-tagline {
  font-size: 12px;
  color: var(--text-tertiary);
  text-transform: uppercase;
  letter-spacing: 3px;
  font-weight: 400;
}

.auth-footer {
  margin-top: 32px;
  font-size: 11px;
  color: var(--text-tertiary);
  letter-spacing: 1px;
}

@keyframes fadeIn {
  from { opacity: 0; transform: translateY(20px); }
  to { opacity: 1; transform: translateY(0); }
}
</style>