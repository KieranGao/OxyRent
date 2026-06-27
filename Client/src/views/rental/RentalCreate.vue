<template>
  <div class="page-container centered">
    <div class="glass-card" style="max-width: 480px; width: 100%;">
      <div class="glass-card-header">
        <h3>创建租赁订单</h3>
      </div>
      <div class="glass-card-body padded">
      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        label-position="top"
      >
        <el-form-item label="车辆" prop="vehicle_id">
          <el-select
            v-model="form.vehicle_id"
            placeholder="请选择车辆"
            filterable
            style="width: 100%"
            :loading="vehiclesLoading"
          >
            <el-option
              v-for="v in vehicleOptions"
              :key="v.id"
              :label="`${v.plate_number} - ${v.brand} ${v.model}`"
              :value="v.id"
            />
          </el-select>
        </el-form-item>

        <div class="form-row">
          <el-form-item label="开始日期" prop="start_date" style="flex: 1">
            <el-date-picker
              v-model="form.start_date"
              type="date"
              placeholder="请选择开始日期"
              value-format="YYYY-MM-DD"
              style="width: 100%"
              :disabled-date="(date) => date < new Date()"
            />
          </el-form-item>
          <el-form-item label="结束日期" prop="end_date" style="flex: 1">
            <el-date-picker
              v-model="form.end_date"
              type="date"
              placeholder="请选择结束日期"
              value-format="YYYY-MM-DD"
              style="width: 100%"
              :disabled-date="(date) => date < new Date(form.start_date || undefined)"
            />
          </el-form-item>
        </div>

        <el-form-item label="支付方式" prop="payment_method">
          <el-radio-group v-model="form.payment_method">
            <el-radio value="balance">余额支付</el-radio>
            <el-radio value="qr">扫码支付</el-radio>
          </el-radio-group>
        </el-form-item>

        <template v-if="form.payment_method === 'balance'">
          <el-form-item label="预估费用">
            <span style="font-weight: 600; color: var(--text-primary);">
              {{ formatMoney(estimatedCost) }}
            </span>
            <span v-if="totalDays > 0" style="color: var(--text-tertiary); font-size: 12px; margin-left: 8px;">
              ({{ totalDays }}天 × {{ formatMoney(selectedVehicle?.daily_rate || 0) }}/天)
            </span>
          </el-form-item>
          <el-form-item label="当前余额">
            <span :style="{ color: balanceEnough ? 'var(--color-success)' : 'var(--color-danger)', fontWeight: 600 }">
              {{ formatMoney(authStore.balance) }}
            </span>
            <span v-if="!balanceEnough && estimatedCost > 0" style="color: var(--color-danger); font-size: 12px; margin-left: 8px;">
              余额不足，差 {{ formatMoney(estimatedCost - authStore.balance) }}
            </span>
          </el-form-item>
        </template>

        <el-form-item label="备注">
          <el-input v-model="form.notes" type="textarea" :rows="3" placeholder="选填" />
        </el-form-item>

        <el-form-item>
          <el-button type="primary" :loading="submitting" @click="handleSubmit">
            创建订单
          </el-button>
          <el-button @click="$router.back()">取消</el-button>
        </el-form-item>
      </el-form>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { getVehicleList } from '@/api/vehicle'
import { createRental } from '@/api/rental'
import { getBalance } from '@/api/user'
import { ElMessage } from 'element-plus'

const route = useRoute()
const router = useRouter()
const authStore = useAuthStore()
const formRef = ref(null)
const submitting = ref(false)
const vehiclesLoading = ref(false)
const vehicleOptions = ref([])

function formatMoney(val) {
  return (val || 0).toLocaleString('zh-CN', { style: 'currency', currency: 'CNY' })
}

async function fetchBalance() {
  try {
    const res = await getBalance()
    if (res.error === 0) {
      authStore.setBalance(res.balance || 0)
    }
  } catch {
    // 忽略
  }
}

const form = reactive({
  vehicle_id: route.query.vehicle_id ? Number(route.query.vehicle_id) : null,
  start_date: '',
  end_date: '',
  payment_method: 'qr',
  notes: '',
})

const rules = {
  vehicle_id: [{ required: true, message: '请选择车辆', trigger: 'change' }],
  start_date: [{ required: true, message: '请选择开始日期', trigger: 'change' }],
  end_date: [{ required: true, message: '请选择结束日期', trigger: 'change' }],
  payment_method: [{ required: true, message: '请选择支付方式', trigger: 'change' }],
}

const selectedVehicle = computed(() =>
  vehicleOptions.value.find(v => v.id === form.vehicle_id)
)

const totalDays = computed(() => {
  if (!form.start_date || !form.end_date) return 0
  const start = new Date(form.start_date)
  const end = new Date(form.end_date)
  const diff = Math.ceil((end - start) / (1000 * 60 * 60 * 24))
  return Math.max(diff, 1)
})

const estimatedCost = computed(() => {
  if (!selectedVehicle.value || totalDays.value <= 0) return 0
  return totalDays.value * (selectedVehicle.value.daily_rate || 0)
})

const balanceEnough = computed(() =>
  authStore.balance >= estimatedCost.value
)

async function loadVehicles() {
  vehiclesLoading.value = true
  try {
    const res = await getVehicleList({ page: 1, page_size: 100, status: 'available' })
    if (res.error === 0) {
      vehicleOptions.value = res.list || res.vehicles || []
    }
  } catch {
    // 忽略
  } finally {
    vehiclesLoading.value = false
  }
}

async function handleSubmit() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  if (form.payment_method === 'balance') {
    if (!balanceEnough.value) {
      ElMessage.error(`余额不足，预估费用 ${formatMoney(estimatedCost.value)}，当前余额 ${formatMoney(authStore.balance)}`)
      return
    }
  }

  submitting.value = true
  try {
    const res = await createRental({
      user_id: authStore.uid,
      vehicle_id: form.vehicle_id,
      start_date: form.start_date,
      end_date: form.end_date,
      payment_method: form.payment_method,
      notes: form.notes,
    })
    if (res.error === 0) {
      ElMessage.success('租赁订单已创建')
      if (form.payment_method === 'balance') {
        authStore.setBalance(authStore.balance - estimatedCost.value)
      }
      router.push('/rentals')
    } else {
      ElMessage.error(res.msg || '创建订单失败')
    }
  } catch {
    ElMessage.error('创建订单失败')
  } finally {
    submitting.value = false
  }
}

onMounted(() => {
  loadVehicles()
  fetchBalance()
})
</script>

<style scoped>
.centered {
  display: flex;
  justify-content: center;
  align-items: flex-start;
  padding-top: 80px;
}
.form-row {
  display: flex;
  gap: 20px;
}
</style>
